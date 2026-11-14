// Entry of the game code library (runtime/gamelib.h), compiled into it with the generated code: the module layout,
// function tables and the game version's sites (the generated layout.c), found by name when the app loads the library.
// Everything else in it stays hidden.
#include "gamelib.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#if defined(_WIN32)
#include <windows.h>
#else
#include <unistd.h>
#include <sys/mman.h>
#endif

/* Stamp Guest Probe v3: diagnostic code compiled INTO swgame.dll.
 * No runtime stamp_flow API, control files, save writes, or gameplay patches.
 * Only guest addresses with verified instructions are hooked by patches.py.
 */
volatile int sw_stamp_guest_enabled = 0;
static FILE *sw_guest_log;
static int sw_guest_initialized;

#define SW_GUEST_SITES 240u
#define SW_GUEST_FNS 4096u
#define SW_GUEST_SITE_MAX 36u
#define SW_GUEST_FN_MAX 16u
#define SW_GUEST_GLOBAL_MAX 30000u

static unsigned long long sw_guest_hits[SW_GUEST_SITES];
static unsigned long long sw_guest_last[SW_GUEST_SITES];
static unsigned sw_guest_samples[SW_GUEST_SITES];
static unsigned long long sw_guest_fns[SW_GUEST_FNS];
static unsigned long long sw_guest_fn_hits[SW_GUEST_FNS];
static unsigned long long sw_guest_fn_last_x1[SW_GUEST_FNS];
static unsigned sw_guest_fn_samples[SW_GUEST_FNS];
static unsigned long long sw_guest_sequence;
static unsigned sw_guest_printed;
static unsigned sw_guest_menu_epoch;

static int sw_guest_log_budget(unsigned *site_samples, unsigned cap) {
    if (__atomic_load_n(&sw_guest_printed, __ATOMIC_RELAXED) >= SW_GUEST_GLOBAL_MAX) return 0;
    unsigned old = __atomic_fetch_add(site_samples, 1u, __ATOMIC_RELAXED);
    if (old >= cap) return 0;
    unsigned total = __atomic_fetch_add(&sw_guest_printed, 1u, __ATOMIC_RELAXED);
    return total < SW_GUEST_GLOBAL_MAX;
}

/* Events use stdout/stderr already captured by Savage Wind; no separate report files.
 * All x-values are untyped raw guest registers, NOT validated stamp counts. */
void sw_stamp_guest_site(unsigned id, uint64_t pc, unsigned kind, unsigned imm,
                         const Cpu *c, uint64_t operand) {
    if (!sw_stamp_guest_enabled || !c || id >= SW_GUEST_SITES) return;
    unsigned epoch = __atomic_load_n(&sw_guest_menu_epoch, __ATOMIC_RELAXED);
    if (kind == 3u && pc == 0x7000dd8208ull)
        epoch = __atomic_add_fetch(&sw_guest_menu_epoch, 1u, __ATOMIC_RELAXED);
    /* Initial UI references occur before loading the save. Do not arm CMP probes
     * until the first observed opening of the stamp menu. */
    if (!epoch && kind != 3u) return;
    unsigned long long n = __atomic_add_fetch(&sw_guest_hits[id], 1ull, __ATOMIC_RELAXED);
    unsigned long long prev = __atomic_exchange_n(&sw_guest_last[id], operand, __ATOMIC_RELAXED);
    if (kind != 3u && n > 3ull && n != 10ull && n != 100ull && prev == operand) return;
    if (!sw_guest_log_budget(&sw_guest_samples[id],
                             kind == 3u ? 32u : SW_GUEST_SITE_MAX)) return;
    unsigned long long seq = __atomic_add_fetch(&sw_guest_sequence, 1ull, __ATOMIC_RELAXED);
    fprintf(sw_guest_log ? sw_guest_log : stderr, "[stamp-guest] seq=%llu epoch=%u type=%s pc=%#llx hits=%llu "
            "operand=%llu imm=%u lr=%#llx nzcv=%#llx "
            "x0=%#llx x1=%#llx x2=%#llx x3=%#llx x4=%#llx x5=%#llx "
            "x8=%#llx x9=%#llx x10=%#llx x19=%#llx x20=%#llx\n",
            seq, epoch, kind == 3u ? "ui" : kind == 2u ? "mov" : "cmp",
            (unsigned long long)pc, n, (unsigned long long)operand, imm,
            (unsigned long long)c->x[30], (unsigned long long)c->nzcv,
            (unsigned long long)c->x[0], (unsigned long long)c->x[1],
            (unsigned long long)c->x[2], (unsigned long long)c->x[3],
            (unsigned long long)c->x[4], (unsigned long long)c->x[5],
            (unsigned long long)c->x[8], (unsigned long long)c->x[9],
            (unsigned long long)c->x[10], (unsigned long long)c->x[19],
            (unsigned long long)c->x[20]);
}

/* Selected guest function entries surrounding the known map-stamp UI references
 * and one earlier pair of candidate comparisons. No global FN_ENTER profiling. */
void sw_stamp_guest_entry(uint64_t pc, const Cpu *c) {
    if (!sw_stamp_guest_enabled || !c ||
        !__atomic_load_n(&sw_guest_menu_epoch, __ATOMIC_RELAXED)) return;
    unsigned slot = (unsigned)((pc ^ (pc >> 13) ^ (pc >> 27)) & (SW_GUEST_FNS - 1u));
    unsigned i;
    for (i = 0; i < 16u; ++i) {
        unsigned pos = (slot + i) & (SW_GUEST_FNS - 1u);
        unsigned long long old = __atomic_load_n(&sw_guest_fns[pos], __ATOMIC_RELAXED);
        if (!old) {
            unsigned long long expected = 0;
            __atomic_compare_exchange_n(&sw_guest_fns[pos], &expected, pc, 0,
                                        __ATOMIC_RELAXED, __ATOMIC_RELAXED);
            old = __atomic_load_n(&sw_guest_fns[pos], __ATOMIC_RELAXED);
        }
        if (old != pc) continue;
        unsigned long long n = __atomic_add_fetch(&sw_guest_fn_hits[pos], 1ull, __ATOMIC_RELAXED);
        unsigned long long prev = __atomic_exchange_n(&sw_guest_fn_last_x1[pos], c->x[1], __ATOMIC_RELAXED);
        if (n > 3 && n != 10 && n != 100 && n != 1000 && prev == c->x[1]) return;
        if (!sw_guest_log_budget(&sw_guest_fn_samples[pos], SW_GUEST_FN_MAX)) return;
        unsigned long long seq = __atomic_add_fetch(&sw_guest_sequence, 1ull, __ATOMIC_RELAXED);
        fprintf(sw_guest_log ? sw_guest_log : stderr, "[stamp-guest] seq=%llu epoch=%u type=fn pc=%#llx hits=%llu "
                "lr=%#llx x0=%#llx x1=%#llx x2=%#llx x3=%#llx x4=%#llx x5=%#llx\n",
                seq, __atomic_load_n(&sw_guest_menu_epoch, __ATOMIC_RELAXED),
                (unsigned long long)pc, n, (unsigned long long)c->x[30],
                (unsigned long long)c->x[0], (unsigned long long)c->x[1],
                (unsigned long long)c->x[2], (unsigned long long)c->x[3],
                (unsigned long long)c->x[4], (unsigned long long)c->x[5]);
        return;
    }
}

/* Stamp follow-up v6: read-only snapshots at UI references + bounded function
 * entry trace around the known map/stamp interface.  The guest owner address
 * captured by v5 remains a *candidate*: a later read is validated against
 * mapped readable host pages, but cannot rule out a concurrent unmap.  No
 * game-state writes, patches to insertion branches, or save modifications.
 */
volatile int sw_stamp_followup_enabled;
static FILE *sw_stamp_followup_file;
static unsigned long long sw_stamp_followup_seq;
static unsigned long long sw_stamp_followup_addr;
static unsigned long long sw_stamp_followup_owner;
static unsigned sw_stamp_followup_epoch;
static unsigned sw_stamp_followup_printed;
#define SW_STAMP_V6_SLOTS 2048u
#define SW_STAMP_V6_GLOBAL_MAX 12000u
#define SW_STAMP_V6_FN_MAX 64u
static uint64_t sw_stamp_followup_pcs[SW_STAMP_V6_SLOTS];
static uint64_t sw_stamp_followup_counts[SW_STAMP_V6_SLOTS];
static unsigned sw_stamp_followup_samples[SW_STAMP_V6_SLOTS];

static FILE *sw_stamp_followup_out(void) {
    return sw_stamp_followup_file ? sw_stamp_followup_file : stderr;
}

static int sw_stamp_followup_readable(uintptr_t address, size_t size) {
    if (!address || address > UINTPTR_MAX - size) return 0;
#if defined(_WIN32)
    MEMORY_BASIC_INFORMATION m;
    if (!VirtualQuery((LPCVOID)address, &m, sizeof(m)) || m.State != MEM_COMMIT)
        return 0;
    DWORD p = m.Protect;
    if ((p & (PAGE_NOACCESS | PAGE_GUARD)) ||
        !(p & (PAGE_READONLY | PAGE_READWRITE | PAGE_WRITECOPY |
               PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)))
        return 0;
    uintptr_t start = (uintptr_t)m.BaseAddress;
    return address >= start && size <= (uintptr_t)m.RegionSize - (address - start);
#else
    long page = sysconf(_SC_PAGESIZE);
    if (page <= 0) return 0;
    uintptr_t start = address - address % (uintptr_t)page;
    size_t span = (size_t)(address + size - start);
    size_t count = (span + (size_t)page - 1) / (size_t)page;
    unsigned char vec[4];
    if (count > sizeof(vec)) return 0;
    return mincore((void *)start, count * (size_t)page, vec) == 0;
#endif
}

static void sw_stamp_followup_snapshot(uint32_t *stamps, uint32_t *pins,
                                      unsigned long long *addr, int *valid) {
    *stamps = *pins = 0; *valid = 0;
    *addr = __atomic_load_n(&sw_stamp_followup_addr, __ATOMIC_RELAXED);
    if (!sw_stamp_followup_readable((uintptr_t)*addr, 8)) return;
#if defined(_WIN32)
    /* ReadProcessMemory gracefully fails on a concurrent unmap; a raw host
     * dereference could fault while the game is running on another thread. */
    uint64_t pair = 0;
    SIZE_T read_bytes = 0;
    if (!ReadProcessMemory(GetCurrentProcess(), (LPCVOID)(uintptr_t)*addr,
                           &pair, sizeof(pair), &read_bytes) ||
        read_bytes != sizeof(pair)) return;
    *stamps = (uint32_t)pair;
    *pins = (uint32_t)(pair >> 32);
#else
    *stamps = *(const volatile u32u *)(uintptr_t)*addr;
    *pins = *(const volatile u32u *)(uintptr_t)(*addr + 4);
#endif
    *valid = 1;
}

static int sw_stamp_followup_budget(unsigned *samples, unsigned cap) {
    if (__atomic_load_n(&sw_stamp_followup_printed, __ATOMIC_RELAXED) >=
        SW_STAMP_V6_GLOBAL_MAX) return 0;
    if (samples && __atomic_fetch_add(samples, 1, __ATOMIC_RELAXED) >= cap) return 0;
    return __atomic_fetch_add(&sw_stamp_followup_printed, 1, __ATOMIC_RELAXED)
           < SW_STAMP_V6_GLOBAL_MAX;
}

void sw_stamp_followup_ui(uint64_t pc, const char *tag, const Cpu *c) {
    if (!sw_stamp_followup_enabled || !c) return;
    unsigned epoch = __atomic_load_n(&sw_stamp_followup_epoch, __ATOMIC_RELAXED);
    if (pc == 0x7000dd8208ull)
        epoch = __atomic_add_fetch(&sw_stamp_followup_epoch, 1u, __ATOMIC_RELAXED);
    if (!sw_stamp_followup_budget(NULL, 0)) return;
    uint32_t stamps, pins; unsigned long long addr; int valid;
    sw_stamp_followup_snapshot(&stamps, &pins, &addr, &valid);
    unsigned long long seq = __atomic_add_fetch(&sw_stamp_followup_seq, 1ull, __ATOMIC_RELAXED);
    fprintf(sw_stamp_followup_out(),
            "[stamp-followup] seq=%llu stage=ui epoch=%u pc=%#llx tag=%s "
            "tracked_addr=%#llx readable=%d capacity=%u pins=%u "
            "lr=%#llx x0=%#llx x1=%#llx x2=%#llx x3=%#llx nzcv=%#llx\n",
            seq, epoch, (unsigned long long)pc, tag,
            addr, valid, stamps, pins,
            (unsigned long long)c->x[30], (unsigned long long)c->x[0],
            (unsigned long long)c->x[1], (unsigned long long)c->x[2],
            (unsigned long long)c->x[3], (unsigned long long)c->nzcv);
    fflush(sw_stamp_followup_out());
}

void sw_stamp_followup_entry(uint64_t pc, const Cpu *c) {
    if (!sw_stamp_followup_enabled || !c) return;
    unsigned epoch = __atomic_load_n(&sw_stamp_followup_epoch, __ATOMIC_RELAXED);
    if (!epoch) return;
    unsigned base = (unsigned)((pc ^ (pc >> 11) ^ (pc >> 23)) & (SW_STAMP_V6_SLOTS - 1));
    for (unsigned i = 0; i < 16; ++i) {
        unsigned pos = (base + i) & (SW_STAMP_V6_SLOTS - 1);
        uint64_t old = __atomic_load_n(&sw_stamp_followup_pcs[pos], __ATOMIC_RELAXED);
        if (!old) {
            uint64_t empty = 0;
            __atomic_compare_exchange_n(&sw_stamp_followup_pcs[pos], &empty, pc, 0,
                                        __ATOMIC_RELAXED, __ATOMIC_RELAXED);
            old = __atomic_load_n(&sw_stamp_followup_pcs[pos], __ATOMIC_RELAXED);
        }
        if (old != pc) continue;
        unsigned long long hits = __atomic_add_fetch(&sw_stamp_followup_counts[pos],
                                                    1ull, __ATOMIC_RELAXED);
        /* Sparse powers-of-two sampling keeps later UI interactions visible
         * rather than exhausting per-function budgets during initialization. */
        int power_of_two = (hits & (hits - 1)) == 0;
        if (hits > 3 && !power_of_two && hits != 99 && hits != 100 && hits != 101) return;
        if (!sw_stamp_followup_budget(&sw_stamp_followup_samples[pos], SW_STAMP_V6_FN_MAX)) return;
        uint32_t stamps, pins; unsigned long long addr; int valid;
        sw_stamp_followup_snapshot(&stamps, &pins, &addr, &valid);
        unsigned long long seq = __atomic_add_fetch(&sw_stamp_followup_seq, 1ull, __ATOMIC_RELAXED);
        fprintf(sw_stamp_followup_out(),
                "[stamp-followup] seq=%llu stage=fn epoch=%u pc=%#llx hits=%llu "
                "tracked_addr=%#llx readable=%d capacity=%u pins=%u "
                "lr=%#llx x0=%#llx x1=%#llx x2=%#llx x3=%#llx nzcv=%#llx\n",
                seq, epoch, (unsigned long long)pc, hits, addr, valid,
                stamps, pins, (unsigned long long)c->x[30],
                (unsigned long long)c->x[0], (unsigned long long)c->x[1],
                (unsigned long long)c->x[2], (unsigned long long)c->x[3],
                (unsigned long long)c->nzcv);
        fflush(sw_stamp_followup_out());
        return;
    }
}

/* Stamp Capacity Trace v5: this instrumentation is compiled into swgame.dll,
 * not merely the host executable. Default ON for the diagnostic release.
 * SAVAGEWIND_STAMP_CAPACITY_TRACE=0 disables logging. It does not edit saves
 * or change the guest instructions beyond the existing optional v4 cheat.
 *
 * Separate output is intentional: swgame.dll may have its own C runtime and
 * stderr stream. The file is created on swgame_tables() even if the patched
 * initializer is never reached; this distinguishes absent execution from an
 * inactive trace. One pair of events is emitted for each execution.
 */
static int sw_stamp_capacity_trace_enabled;
static FILE *sw_stamp_capacity_file;
static unsigned long long sw_stamp_capacity_seq;

static FILE *sw_stamp_capacity_out(void) {
    return sw_stamp_capacity_file ? sw_stamp_capacity_file : stderr;
}

void sw_stamp_capacity_choice(uint64_t pc, int enabled, uint64_t chosen,
                              uint64_t owner, uint64_t lr) {
    if (!sw_stamp_capacity_trace_enabled) return;
    unsigned long long seq = __atomic_add_fetch(&sw_stamp_capacity_seq, 1ull, __ATOMIC_RELAXED);
    fprintf(sw_stamp_capacity_out(),
            "[stamp-capacity] seq=%llu stage=choice pc=%#llx reached=1 "
            "enabled_return=%d chosen=%llu owner=%#llx lr=%#llx\n",
            seq, (unsigned long long)pc, enabled, (unsigned long long)chosen,
            (unsigned long long)owner, (unsigned long long)lr);
    fflush(sw_stamp_capacity_out());
}

void sw_stamp_capacity_stored(uint64_t pc, uint64_t address, uint64_t written,
                              uint32_t read_stamps, uint32_t read_pins, uint64_t lr) {
    __atomic_store_n(&sw_stamp_followup_addr, address, __ATOMIC_RELAXED);
    __atomic_store_n(&sw_stamp_followup_owner, address - 0x228ull, __ATOMIC_RELAXED);
    if (!sw_stamp_capacity_trace_enabled) return;
    unsigned long long seq = __atomic_add_fetch(&sw_stamp_capacity_seq, 1ull, __ATOMIC_RELAXED);
    unsigned expected = (uint32_t)written;
    unsigned pins_expected = (unsigned)(written >> 32);
    fprintf(sw_stamp_capacity_out(),
            "[stamp-capacity] seq=%llu stage=store pc=%#llx stored_addr=%#llx "
            "written_x8=%#llx expected_stamps=%u observed_stamps=%u "
            "expected_pins=%u observed_pins=%u matches=%d lr=%#llx\n",
            seq, (unsigned long long)pc, (unsigned long long)address,
            (unsigned long long)written, expected, (unsigned)read_stamps,
            pins_expected, (unsigned)read_pins,
            read_stamps == expected && read_pins == pins_expected,
            (unsigned long long)lr);
    fflush(sw_stamp_capacity_out());
}

extern const GameLibModule game_modules[];
extern const uint32_t game_module_count;
extern const char game_version[];
extern const GameLibSite game_sites[];
extern const uint32_t game_site_count;

GAMELIB_EXPORT const GameLibTables *swgame_tables(void) {
    if (!sw_guest_initialized) {
        sw_guest_initialized = 1;
        const char *followup_trace = getenv("SAVAGEWIND_STAMP_FOLLOWUP_TRACE");
        sw_stamp_followup_enabled = !followup_trace || strcmp(followup_trace, "0") != 0;
        if (sw_stamp_followup_enabled) {
            sw_stamp_followup_file = fopen("stamp-followup-v6.log", "wb");
            if (sw_stamp_followup_file) setvbuf(sw_stamp_followup_file, NULL, _IONBF, 0);
            fprintf(sw_stamp_followup_out(),
                    "[stamp-followup] v6 armed build=cd57b23fa4bbad65 "
                    "read_only=1 fn_budget=%u global_budget=%u "
                    "tracked_addr=0 (waiting for capacity initializer)\n",
                    SW_STAMP_V6_FN_MAX, SW_STAMP_V6_GLOBAL_MAX);
            fflush(sw_stamp_followup_out());
            fprintf(stderr, "[stamp-followup] v6 armed; output=%s\n",
                    sw_stamp_followup_file ? "stamp-followup-v6.log" : "stderr (file open failed)");
        }
        const char *capacity_trace = getenv("SAVAGEWIND_STAMP_CAPACITY_TRACE");
        sw_stamp_capacity_trace_enabled = !capacity_trace || strcmp(capacity_trace, "0") != 0;
        if (sw_stamp_capacity_trace_enabled) {
            sw_stamp_capacity_file = fopen("stamp-capacity-v5.log", "wb");
            if (sw_stamp_capacity_file) setvbuf(sw_stamp_capacity_file, NULL, _IONBF, 0);
            fprintf(sw_stamp_capacity_out(),
                    "[stamp-capacity] v5 armed; swgame.dll loaded; "
                    "choice=0x7000d844e0 store=0x7000d844f4; "
                    "awaiting initializer execution (check Build ID 1.9.0)\n");
            fflush(sw_stamp_capacity_out());
            fprintf(stderr, "[stamp-capacity] v5 armed; output=%s\n",
                    sw_stamp_capacity_file ? "stamp-capacity-v5.log" : "stderr (file open failed)");
        }
        const char *trace = getenv("SAVAGEWIND_STAMP_GUEST_TRACE");
        sw_stamp_guest_enabled = trace && !strcmp(trace, "1");
        if (sw_stamp_guest_enabled) {
            /* Independent game-DLL output: works even if its C library has a
             * separate stderr FILE* from the MSYS2/MinGW Windows executable. */
            sw_guest_log = fopen("stamp-guest-v3.log", "wb");
            if (sw_guest_log) setvbuf(sw_guest_log, NULL, _IONBF, 0);
            FILE *out = sw_guest_log ? sw_guest_log : stderr;
            fprintf(out, "[stamp-guest] enabled v3 (swgame.dll instruction hooks; read-only)\n");
            fprintf(stderr, "[stamp-guest] v3 enabled; output=%s\n",
                    sw_guest_log ? "stamp-guest-v3.log" : "stderr (file open failed)");
        }
    }
    static const GameLibTables t = {GAMELIB_ABI, GAMELIB_ID, game_modules, &game_module_count,
                                    game_version, game_sites, &game_site_count};
    return &t;
}
