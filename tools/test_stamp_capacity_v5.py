"""Unit/end-to-end tests for stamp-capacity v5 diagnostics.

The compiled harness uses synthetic CPU state and guest memory. No game data.
SAVAGEWIND_TEST_MAIN_NSO optionally checks actual original NSO opcodes.
"""
from pathlib import Path
from types import SimpleNamespace
import os
import re
import shutil
import struct
import subprocess
import tempfile
import unittest

import patches

ROOT = Path(__file__).resolve().parent.parent
BASE = 0x7000000000
SITE = 0x7000D844E0
STORE = 0x7000D844F4
BUILD_ID = 'cd57b23fa4bbad65'


def fake_gen(build=BUILD_ID, changed=None):
    buf = bytearray((STORE - BASE) + 12)
    for address, word in ((SITE, 0xD2800C88), (SITE+4, 0xF2C000A8),
                          (STORE, 0xF9011668), (STORE+4, 0xB9023275)):
        struct.pack_into('<I', buf, address - BASE,
                         (changed or {}).get(address, word))
    return SimpleNamespace(base=BASE, nso=SimpleNamespace(build_id=build, image=buf))


class StampCapacityV5Tests(unittest.TestCase):
    def test_hard_guard_and_two_sites(self):
        table = patches.table({}, BUILD_ID)
        self.assertIs(table[SITE], patches._stamp_capacity_9999)
        self.assertIs(table[STORE], patches._stamp_capacity_store_probe)
        self.assertIn(0x7000B37120, table)  # motorcycle cheat
        for bid in (None, '0' * 16, 'de0b6ac4efc1dda7'):
            self.assertNotIn(SITE, patches.table({}, bid))
            self.assertNotIn(STORE, patches.table({}, bid))
        for where in (SITE, SITE + 4, STORE, STORE + 4):
            bad = fake_gen(changed={where: 0})
            with self.subTest(where=hex(where)):
                with self.assertRaises(AssertionError):
                    patches._stamp_capacity_9999(bad, SITE)
                with self.assertRaises(AssertionError):
                    patches._stamp_capacity_store_probe(bad, STORE)

    def test_store_hook_exact_replay_and_readback(self):
        chosen = patches._stamp_capacity_9999(fake_gen(), SITE)
        stored = patches._stamp_capacity_store_probe(fake_gen(), STORE)
        self.assertEqual(chosen.count('rt_sw_stamps_9999_enabled()'), 1)
        self.assertIn('sw_stamp_capacity_choice(', chosen)
        self.assertIn('ST64(addr, val);', stored)
        self.assertIn('const volatile u32u', stored)
        self.assertIn('(addr + 4)', stored)
        self.assertIn('sw_stamp_capacity_stored(', stored)
        self.assertNotIn('c->nzcv =', chosen + stored)
        self.assertNotIn('c->x[19] =', chosen + stored)

    def test_end_to_end_compiled_hooks_and_file(self):
        cc = shutil.which('clang') or shutil.which('gcc')
        if not cc:
            self.skipTest('C compiler unavailable')
        choice = patches._stamp_capacity_9999(fake_gen(), SITE)
        store = patches._stamp_capacity_store_probe(fake_gen(), STORE)
        source = r'''
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "gamelib.h"
const GameLibModule game_modules[1] = {0};
const uint32_t game_module_count = 0;
const char game_version[] = "1.9.0";
const GameLibSite game_sites[1] = {0};
const uint32_t game_site_count = 0;
extern const GameLibTables *swgame_tables(void);
static int active;
int rt_sw_stamps_9999_enabled(void) { return active; }
static void run(int on) {
    uint8_t storage[0x400] = {0};
    Cpu cpu = {0}; Cpu *c = &cpu;
    c->x[19] = (uint64_t)(uintptr_t)storage;
    c->x[30] = 0x7000d8397cull;
    c->nzcv = 0xa0000000ull;
    active = on;
    %s
    assert(c->x[8] == (on ? 9999ull : 100ull));
    /* Real MOVK X8, #5, LSL #32 stays in generated code. */
    c->x[8] = (c->x[8] & ~(0xffffull << 32)) | (5ull << 32);
    %s
    assert(LD32(c->x[19] + 0x228) == (uint32_t)(on ? 9999 : 100));
    assert(LD32(c->x[19] + 0x22c) == 5u);
    assert(c->x[30] == 0x7000d8397cull);
    assert(c->nzcv == 0xa0000000ull);
    assert(c->x[8] == ((5ull << 32) | (on ? 9999ull : 100ull)));
    c->x[21] = 1u;
    ST32(c->x[19] + 0x230, c->x[21]);
    assert(LD32(c->x[19] + 0x230) == 1u);
}
int main(void) {
    assert(swgame_tables()->abi == GAMELIB_ABI);
    run(0);
    run(1);
    return 0;
}
''' % (choice, store)
        with tempfile.TemporaryDirectory() as d:
            binary = Path(d) / 'harness'
            src = Path(d) / 'harness.c'
            src.write_text(source)
            cmd = [cc, '-std=gnu11', '-O1', '-Wall', '-Wextra', '-Werror',
                   '-Iruntime', '-DGAMELIB_ID="test"',
                   'tools/gamelib_entry.c', str(src), '-o', str(binary)]
            build = subprocess.run(cmd, cwd=ROOT, text=True, capture_output=True)
            self.assertEqual(build.returncode, 0, build.stderr)
            env = os.environ.copy()
            env.pop('SAVAGEWIND_STAMP_CAPACITY_TRACE', None)  # default ON
            result = subprocess.run([str(binary)], cwd=d, env=env,
                                    text=True, capture_output=True, check=True)
            log = Path(d) / 'stamp-capacity-v5.log'
            self.assertTrue(log.exists(), result.stderr)
            lines = log.read_text().splitlines()
            self.assertIn('v5 armed', lines[0])
            self.assertEqual(len(lines), 5)
            self.assertIn('stage=choice', lines[1]); self.assertIn('reached=1', lines[1])
            self.assertIn('enabled_return=0 chosen=100', lines[1])
            self.assertIn('stage=store', lines[2])
            self.assertIn('expected_stamps=100 observed_stamps=100', lines[2])
            self.assertIn('expected_pins=5 observed_pins=5 matches=1', lines[2])
            self.assertIn('enabled_return=1 chosen=9999', lines[3])
            self.assertIn('expected_stamps=9999 observed_stamps=9999', lines[4])
            self.assertTrue(all('matches=1' in a for a in (lines[2], lines[4])))
            self.assertEqual([int(re.search(r'seq=(\d+)', a).group(1)) for a in lines[1:]],
                             [1, 2, 3, 4])
            env['SAVAGEWIND_STAMP_CAPACITY_TRACE'] = '0'
            log.unlink()
            subprocess.run([str(binary)], cwd=d, env=env, capture_output=True, check=True)
            self.assertFalse(log.exists())

    def test_real_user_nso_if_available(self):
        target = os.environ.get('SAVAGEWIND_TEST_MAIN_NSO')
        if not target:
            self.skipTest('SAVAGEWIND_TEST_MAIN_NSO not set')
        from nso import Nso
        n = Nso(target)
        self.assertEqual(n.build_id[:16], BUILD_ID)
        gen = SimpleNamespace(base=BASE, nso=n)
        patches._stamp_capacity_9999(gen, SITE)
        patches._stamp_capacity_store_probe(gen, STORE)


if __name__ == '__main__':
    unittest.main()
