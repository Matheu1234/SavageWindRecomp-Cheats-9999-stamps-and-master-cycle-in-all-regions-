/* Pure host test for the probe compiled with the generated game DLL. */
#include "gamelib.h"
#include <stdio.h>
#include <stdint.h>

const GameLibModule game_modules[1] = {{0}};
const uint32_t game_module_count = 0;
const char game_version[] = "1.9.0";
const GameLibSite game_sites[1] = {{0}};
const uint32_t game_site_count = 0;

extern const GameLibTables *swgame_tables(void);
extern volatile int sw_stamp_guest_enabled;
extern void sw_stamp_guest_site(unsigned, uint64_t, unsigned, unsigned, const Cpu *, uint64_t);
extern void sw_stamp_guest_entry(uint64_t, const Cpu *);

int main(void) {
    Cpu c = {0};
    c.x[0] = 0xabcd;
    c.x[1] = 4;
    c.x[30] = 0x7000dd8204ull;
    const GameLibTables *tables = swgame_tables();
    if (!tables || tables->abi != GAMELIB_ABI) return 20;
    if (!sw_stamp_guest_enabled) return 0;
    /* No generic comparison should be logged before the UI anchor. */
    sw_stamp_guest_site(0u, 0x7000d91408ull, 1u, 100u, &c, 42u);
    sw_stamp_guest_site(212u, 0x7000dd8208ull, 3u, 0u, &c, 0u);
    sw_stamp_guest_site(0u, 0x7000d91408ull, 1u, 100u, &c, 42u);
    sw_stamp_guest_site(0u, 0x7000d91408ull, 1u, 100u, &c, 43u);
    sw_stamp_guest_entry(0x7000dd8140ull, &c);
    c.x[1] = 5;
    sw_stamp_guest_entry(0x7000dd8140ull, &c);
    sw_stamp_guest_site(212u, 0x7000dd8208ull, 3u, 0u, &c, 0u);
    return 0;
}
