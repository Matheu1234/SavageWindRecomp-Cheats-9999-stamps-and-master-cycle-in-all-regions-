/* Standalone runtime-only test: no game code, SDL or guest save required. */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "cpu.h"
#include "stamp_flow.h"

volatile int g_fnprof;
static Cpu fake;
Cpu *kernel_current_cpu(void) { return &fake; }

static void command(const char *s) {
    FILE *f = fopen("stamp-flow-control.txt", "wb");
    assert(f);
    assert(fputs(s, f) >= 0);
    assert(fclose(f) == 0);
}

int main(void) {
    rt_sw_stamp_flow_init();
    assert(!g_fnprof && !rt_sw_stamp_flow_active);
    command("baseline\n");
    rt_sw_stamp_flow_frame(15);
    assert(g_fnprof && rt_sw_stamp_flow_active);
    fake.x[1] = 0xabc;
    rt_sw_stamp_flow_hit(0x7000dd8208ull, 0x7000dd81e0ull, 5);
    rt_sw_stamp_flow_hit(0x7000dd8208ull, 0x7000dd81e0ull, 6);
    rt_sw_stamp_flow_hit(0x7000dd8208ull, 0x7000dd81e4ull, 7);
    command("add\n");
    rt_sw_stamp_flow_frame(30);
    assert(g_fnprof && rt_sw_stamp_flow_active);
    rt_sw_stamp_flow_hit(0x7000dd8208ull, 0x7000dd81e0ull, 8);
    rt_sw_stamp_flow_hit(0x7000da7788ull, 0x7000da7780ull, 9);
    command("remove\n");
    rt_sw_stamp_flow_frame(45);
    rt_sw_stamp_flow_hit(0x7000da7788ull, 0x7000da7770ull, 11);
    command("idle\n");
    rt_sw_stamp_flow_frame(60);
    assert(!g_fnprof && !rt_sw_stamp_flow_active);
    rt_sw_stamp_flow_hit(0x7000dd8208ull, 0x7000dd81e0ull, 12); /* ignored */
    rt_sw_stamp_flow_shutdown();
    puts("OK: 3 labeled phases emitted; no game data modified");
    return 0;
}
