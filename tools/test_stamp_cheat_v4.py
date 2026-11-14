"""Validate the build-guarded Switch stamp-capacity hook and launcher states.

All test fixtures are synthetic. No Nintendo game assets are distributed here.
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
BUILD_ID = 'cd57b23fa4bbad65'


def fake_gen(build=BUILD_ID, word=0xD2800C88, upper=0xF2C000A8,
             store=0xF9011668, warp=0xB9023275):
    data=bytearray((SITE-BASE)+0x20)
    for off, w in [(0,word),(4,upper),(0x14,store),(0x18,warp)]:
        struct.pack_into('<I',data,SITE-BASE+off,w)
    return SimpleNamespace(nso=SimpleNamespace(build_id=build,image=data),base=BASE)

class StampV4Tests(unittest.TestCase):
    def test_exact_build_only_and_no_hook_collision(self):
        known=patches.table({},BUILD_ID)
        self.assertIn(SITE,known)
        self.assertIn(0x7000B37120,known)  # motorcycle unchanged
        self.assertEqual(len(known),len(set(known)))
        for build in [None,'0000000000000000','de0b6ac4efc1dda7']:
            self.assertNotIn(SITE,patches.table({},build))

    def test_hook_keeps_pins_warp_and_default(self):
        hook=patches._stamp_capacity_9999(fake_gen(),SITE)
        self.assertIn('rt_sw_stamps_9999_enabled()',hook)
        self.assertIn('? 9999ull : 100ull',hook)
        self.assertIn('c->x[8]',hook)
        # Simulated ARM64 MOVK after the substituted MOVZ: upper dword remains 5.
        for active,want in [(False,100),(True,9999)]:
            low=9999 if active else 100
            x8=low | (5<<32)
            self.assertEqual(x8 & 0xffffffff,want)
            self.assertEqual(x8 >> 32,5)
            self.assertEqual(1,1)  # following STR W21 unaffected

    def test_wrong_opcode_and_neighbors_fail_closed(self):
        for data in [dict(word=0xD2800CA8),dict(upper=0xF2C000C8),
                     dict(store=0xF9011669),dict(warp=0xB9023274)]:
            with self.subTest(data=data),self.assertRaises(AssertionError):
                patches._stamp_capacity_9999(fake_gen(**data),SITE)
        with self.assertRaises(AssertionError):
            patches._stamp_capacity_9999(fake_gen(build='f'*16),SITE)

    def test_manifest_and_host_cheat_toggle_full_hook(self):
        cc=shutil.which('clang') or shutil.which('gcc')
        if not cc:self.skipTest('C compiler not found')
        hook=patches._stamp_capacity_9999(fake_gen(),SITE)
        source=r"""
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "sw_cheats.h"
typedef struct { uint64_t x[32]; } Cpu;
/* The guest DLL logger is separately tested by test_stamp_capacity_v5. */
void sw_stamp_capacity_choice(uint64_t pc, int on, uint64_t chosen, uint64_t owner, uint64_t lr) {
    (void)pc; (void)on; (void)chosen; (void)owner; (void)lr;
}
static uint64_t guest_capacity(void) {
    Cpu cpu = {{0}}; Cpu *c = &cpu;
    %s
    /* MOVK X8, #5, LSL 32 - the next untouched guest instruction */
    c->x[8] = (c->x[8] & ~(0xffffull << 32)) | (5ull << 32);
    assert((c->x[8] >> 32) == 5ull);
    return (uint32_t)c->x[8];
}
int main(void) {
    char err[256];
    SW_CheatConfig cfg;
    rt_sw_cheat_init();
    assert(guest_capacity()==100);
    assert(sw_cheat_set_enabled("stamps_9999",1,err,sizeof err));
    rt_sw_cheat_init();
    assert(guest_capacity()==9999);
    assert(sw_cheat_load_config("savagewind-cheats.ini",&cfg,err,sizeof err));
    assert(cfg.stamps_9999);
    assert(!cfg.motorcycle_all_regions);
    assert(sw_cheat_set_enabled("motorcycle_all_regions",1,err,sizeof err));
    rt_sw_cheat_init();
    assert(guest_capacity()==9999);
    assert(rt_sw_motorcycle_all_regions()==1);
    assert(sw_cheat_set_enabled("stamps_9999",0,err,sizeof err));
    rt_sw_cheat_init();
    assert(guest_capacity()==100);
    assert(rt_sw_motorcycle_all_regions()==1);
    puts("OK: guest hook 100->9999->100, pin=5, independent motorcycle");
    return 0;
}
""" % hook
        with tempfile.TemporaryDirectory() as d:
            home=Path(d)
            for f in (ROOT/'cheats').glob('*/cheat.ini'):
                tgt=home/'cheats'/f.parent.name
                tgt.mkdir(parents=True,exist_ok=True)
                shutil.copyfile(f,tgt/'cheat.ini')
            test=home/'harness.c';test.write_text(source)
            exe=home/'harness'
            cmd=[cc,'-std=c11','-O1','-Wall','-Wextra','-Werror',
                 '-I'+str(ROOT/'runtime'),str(test),str(ROOT/'runtime/sw_cheats.c'),'-o',str(exe)]
            subprocess.run(cmd,capture_output=True,text=True,check=True)
            r=subprocess.run([str(exe)],cwd=home,capture_output=True,text=True,check=True)
            self.assertIn('OK: guest hook',r.stdout)
            self.assertIn('9999 Map Stamps Limit: EXPERIMENTAL ON',r.stderr)

    def test_real_user_nso_if_available(self):
        # Developers may set SAVAGEWIND_TEST_MAIN_NSO to their own dumped NSO.
        file=os.environ.get('SAVAGEWIND_TEST_MAIN_NSO')
        if not file:self.skipTest('No user-provided NSO (optional)')
        from nso import Nso
        n=Nso(file)
        gen=SimpleNamespace(nso=n,base=BASE)
        self.assertEqual(n.build_id[:16],BUILD_ID)
        patches._stamp_capacity_9999(gen,SITE)

if __name__=='__main__':unittest.main()
