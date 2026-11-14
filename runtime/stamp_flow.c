/* Savage Wind stamp flow probe, v2.
 *
 * Opt in with SAVAGEWIND_STAMP_FLOW=1. At most once per 15 presented frames,
 * read stamp-flow-control.txt from the Savage Wind data directory. Commands:
 * idle, baseline, add, remove. A phase transition atomically freezes entries
 * and writes stamp-flow-<epoch>-<serial>-<phase>.tsv (no save modifications).
 *
 * Crucially this uses the existing FN_ENTER callbacks: there is NO need to
 * regenerate the guest game code. The program counter is the guest function
 * entry; lr is the guest link register on entry (may be stale on tail calls).
 * All addresses and x0..x5 values are observations, NOT decoded stamp counts.
 */
#include "stamp_flow.h"

#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "cpu.h"
#include "kernel.h"

#define FLOW_SLOTS (1u << 18)
#define FLOW_POLL_FRAMES 15u

typedef struct {
    _Atomic unsigned state;   /* 0 = free, 1 = initializing, 2 = ready */
    uint64_t pc, lr;
    uint64_t x[6];           /* first call for the (function, caller) pair */
    _Atomic uint64_t hits;
} FlowSlot;

static FlowSlot slots[FLOW_SLOTS];
static _Atomic unsigned inflight;
/* Each active phase owns an odd generation. Transitions increment it, so a
 * preempted callback from an older phase cannot write into a new phase. */
static _Atomic uint64_t flow_generation;
static _Atomic uint64_t dropped;
volatile int rt_sw_stamp_flow_active = 0;

static int armed;
static int shutdown_registered;
static char phase[16] = "idle";
static char last_command[16] = "";
static time_t run_started;
static unsigned serial;
static uint64_t start_frame;
extern volatile int g_fnprof;

static uint32_t slot_hash(uint64_t a, uint64_t b) {
    uint64_t h = (a >> 2) ^ (b << 17) ^ (b >> 11);
    h ^= h >> 30;
    h *= UINT64_C(0xbf58476d1ce4e5b9);
    h ^= h >> 27;
    h *= UINT64_C(0x94d049bb133111eb);
    return (uint32_t)(h ^ (h >> 31)) & (FLOW_SLOTS - 1u);
}

void rt_sw_stamp_flow_hit(uint64_t pc, uint64_t guest_lr, uint64_t x0) {
    uint64_t generation = atomic_load_explicit(&flow_generation, memory_order_seq_cst);
    if (!(generation & 1u)) return;
    atomic_fetch_add_explicit(&inflight, 1u, memory_order_seq_cst);
    if (generation != atomic_load_explicit(&flow_generation, memory_order_seq_cst)) goto done;

    uint32_t h = slot_hash(pc, guest_lr);
    for (unsigned tries = 0; tries < FLOW_SLOTS; ++tries, h = (h + 1u) & (FLOW_SLOTS - 1u)) {
        FlowSlot *s = &slots[h];
        unsigned state = atomic_load_explicit(&s->state, memory_order_acquire);
        if (state == 0u) {
            unsigned expected = 0u;
            if (atomic_compare_exchange_strong_explicit(&s->state, &expected, 1u,
                    memory_order_acq_rel, memory_order_acquire)) {
                s->pc = pc;
                s->lr = guest_lr;
                s->x[0] = x0;
                Cpu *c = kernel_current_cpu();
                for (int i = 1; i < 6; ++i) s->x[i] = c ? c->x[i] : 0;
                atomic_store_explicit(&s->hits, 1u, memory_order_relaxed);
                atomic_store_explicit(&s->state, 2u, memory_order_release);
                goto done;
            }
            state = expected;
        }
        /* A competing thread may be initializing this slot; wait for it to
         * publish the key before checking for an empty slot further on. */
        while (state == 1u)
            state = atomic_load_explicit(&s->state, memory_order_acquire);
        if (state == 2u && s->pc == pc && s->lr == guest_lr) {
            atomic_fetch_add_explicit(&s->hits, 1u, memory_order_relaxed);
            goto done;
        }
    }
    atomic_fetch_add_explicit(&dropped, 1u, memory_order_relaxed);
done:
    atomic_fetch_sub_explicit(&inflight, 1u, memory_order_release);
}

static int descending_hits(const void *aa, const void *bb) {
    const FlowSlot *a = *(const FlowSlot * const *)aa;
    const FlowSlot *b = *(const FlowSlot * const *)bb;
    uint64_t ca = atomic_load_explicit(&a->hits, memory_order_relaxed);
    uint64_t cb = atomic_load_explicit(&b->hits, memory_order_relaxed);
    if (ca != cb) return ca < cb ? 1 : -1;
    if (a->pc != b->pc) return a->pc < b->pc ? -1 : 1;
    return a->lr < b->lr ? -1 : a->lr > b->lr ? 1 : 0;
}

/* Name output by epoch and sequence; never intentionally overwrite a trace
 * from a previous game process. */
static FILE *open_report(char *name, size_t capacity, const char *label) {
    for (unsigned suffix = 0; suffix < 1000u; ++suffix) {
        if (suffix)
            snprintf(name, capacity, "stamp-flow-%lld-%04u-%s-%u.tsv",
                     (long long)run_started, serial, label, suffix);
        else
            snprintf(name, capacity, "stamp-flow-%lld-%04u-%s.tsv",
                     (long long)run_started, serial, label);
        FILE *old = fopen(name, "rb");
        if (old) { fclose(old); continue; }
        return fopen(name, "wb");
    }
    return NULL;
}

static void end_phase(uint64_t frame) {
    if (!strcmp(phase, "idle")) return;
    /* Prevent new callbacks, and let already-entered callbacks complete. */
    atomic_fetch_add_explicit(&flow_generation, 1u, memory_order_seq_cst); /* odd -> even */
    __atomic_store_n(&rt_sw_stamp_flow_active, 0, __ATOMIC_RELEASE);
    __atomic_store_n(&g_fnprof, 0, __ATOMIC_RELEASE);
    while (atomic_load_explicit(&inflight, memory_order_acquire)) { /* brief drain */ }

    size_t rows = 0;
    uint64_t hits = 0;
    for (size_t i = 0; i < FLOW_SLOTS; ++i) {
        if (atomic_load_explicit(&slots[i].state, memory_order_acquire) == 2u) {
            ++rows;
            hits += atomic_load_explicit(&slots[i].hits, memory_order_relaxed);
        }
    }
    FlowSlot **sorted = rows ? malloc(rows * sizeof(*sorted)) : NULL;
    if (rows && !sorted) {
        fprintf(stderr, "[stamp-flow] ERROR: unable to allocate report index; phase=%s lost\n", phase);
    } else {
        size_t j = 0;
        for (size_t i = 0; i < FLOW_SLOTS; ++i)
            if (atomic_load_explicit(&slots[i].state, memory_order_relaxed) == 2u)
                sorted[j++] = &slots[i];
        if (rows) qsort(sorted, rows, sizeof(*sorted), descending_hits);
        char name[160];
        FILE *f = open_report(name, sizeof(name), phase);
        if (!f) {
            fprintf(stderr, "[stamp-flow] ERROR: cannot create report for phase=%s\n", phase);
        } else {
            fprintf(f, "# stamp_flow_v2 phase=%s begin_frame=%llu end_frame=%llu total_hits=%llu "
                       "edges=%zu dropped=%llu\n",
                    phase, (unsigned long long)start_frame, (unsigned long long)frame,
                    (unsigned long long)hits, rows,
                    (unsigned long long)atomic_load_explicit(&dropped, memory_order_relaxed));
            fputs("pc\tlr\thits\tx0\tx1\tx2\tx3\tx4\tx5\n", f);
            for (size_t i = 0; i < rows; ++i) {
                FlowSlot *s = sorted[i];
                fprintf(f, "0x%llx\t0x%llx\t%llu\t0x%llx\t0x%llx\t0x%llx\t0x%llx\t0x%llx\t0x%llx\n",
                        (unsigned long long)s->pc, (unsigned long long)s->lr,
                        (unsigned long long)atomic_load_explicit(&s->hits, memory_order_relaxed),
                        (unsigned long long)s->x[0], (unsigned long long)s->x[1],
                        (unsigned long long)s->x[2], (unsigned long long)s->x[3],
                        (unsigned long long)s->x[4], (unsigned long long)s->x[5]);
            }
            if (fclose(f)) fprintf(stderr, "[stamp-flow] WARNING: error closing %s\n", name);
            fprintf(stderr, "[stamp-flow] end phase=%s edges=%zu calls=%llu dropped=%llu output=%s\n",
                    phase, rows, (unsigned long long)hits,
                    (unsigned long long)atomic_load_explicit(&dropped, memory_order_relaxed), name);
        }
    }
    free(sorted);
    strcpy(phase, "idle");
}

static void begin_phase(const char *command, uint64_t frame) {
    /* No callbacks are running after end_phase(); zero all counters. */
    memset(slots, 0, sizeof slots);
    atomic_store_explicit(&dropped, 0u, memory_order_relaxed);
    /* Do not reset inflight: a late callback from a previous generation may
     * still balance its own increment/decrement after the old phase drained. */
    strncpy(phase, command, sizeof(phase) - 1u);
    phase[sizeof(phase) - 1u] = 0;
    ++serial;
    start_frame = frame;
    atomic_fetch_add_explicit(&flow_generation, 1u, memory_order_seq_cst); /* even -> odd */
    __atomic_store_n(&rt_sw_stamp_flow_active, 1, __ATOMIC_RELEASE);
    __atomic_store_n(&g_fnprof, 1, __ATOMIC_RELEASE);
    fprintf(stderr, "[stamp-flow] begin phase=%s frame=%llu (function-entry sampling)\n",
            phase, (unsigned long long)frame);
}

static void read_command(char *out, size_t cap) {
    FILE *f = fopen("stamp-flow-control.txt", "rb");
    if (!f) { strcpy(out, "idle"); return; }
    size_t len = fread(out, 1, cap - 1u, f);
    fclose(f);
    out[len] = 0;
    for (size_t i = 0; i < len; ++i)
        if (out[i] == '\r' || out[i] == '\n' || out[i] == ' ' || out[i] == '\t') { out[i] = 0; break; }
    if (strcmp(out, "baseline") && strcmp(out, "add") &&
        strcmp(out, "remove") && strcmp(out, "idle")) strcpy(out, "idle");
}

void rt_sw_stamp_flow_init(void) {
    const char *e = getenv("SAVAGEWIND_STAMP_FLOW");
    if (!e || strcmp(e, "1")) return;
    if (getenv("SAVAGEWIND_FNPROF")) {
        fputs("[stamp-flow] ERROR: SAVAGEWIND_FNPROF conflicts with stamp-flow phases; "
              "unset SAVAGEWIND_FNPROF and restart\n", stderr);
        return;
    }
    if (armed) end_phase(0);  /* guest reboot: export the previous phase */
    run_started = time(NULL);
    last_command[0] = 0;     /* allow rearming the same command after reboot */
    armed = 1;
    if (!shutdown_registered) {
        atexit(rt_sw_stamp_flow_shutdown);
        shutdown_registered = 1;
    }
    fprintf(stderr, "[stamp-flow] enabled; control=stamp-flow-control.txt; "
                    "commands=baseline,add,remove,idle; no gameplay changes\n");
}

void rt_sw_stamp_flow_frame(uint64_t frame) {
    if (!armed || frame % FLOW_POLL_FRAMES) return;
    char command[16];
    read_command(command, sizeof command);
    if (!strcmp(command, last_command)) return;
    strcpy(last_command, command);
    end_phase(frame);
    if (strcmp(command, "idle")) begin_phase(command, frame);
}

void rt_sw_stamp_flow_shutdown(void) {
    if (!armed) return;
    armed = 0;
    end_phase(0);   /* 0 denotes process exit rather than a presented frame */
}
