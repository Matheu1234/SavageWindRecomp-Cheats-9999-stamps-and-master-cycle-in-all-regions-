"""v6 follow-up probes: synthetic-memory harness and build/NSO constraints.

Does not distribute guest game code or modify saves.  Optional NSO real check:
SAVAGEWIND_TEST_MAIN_NSO=/path/to/main.
"""
import os
from pathlib import Path
import re
import shutil
import subprocess
import tempfile
import unittest
from types import SimpleNamespace

import patches
import stamp_trace_sites as ts
from test_stamp_capacity_v5 import fake_gen, BUILD_ID, SITE, STORE, ROOT

UI = 0x7000dd8208


class StampFollowupV6Tests(unittest.TestCase):
    def test_real_guest_sites_and_conflict_guards(self):
        self.assertEqual(ts.UI_REFERENCE_SITES[UI][1], 'open_stamp_menu')
        self.assertIn(UI, patches.table({}, BUILD_ID))
        self.assertIs(patches.table({}, BUILD_ID)[SITE], patches._stamp_capacity_9999)
        self.assertIs(patches.table({}, BUILD_ID)[STORE], patches._stamp_capacity_store_probe)
        self.assertIn(0x7000b37120, patches.table({}, BUILD_ID))
        gen = fake_gen()
        gen.lifter = SimpleNamespace(lift=lambda w, a: SimpleNamespace(kind='op', code='c->x[1] = c->x[1];'))
        # UI hook's ADRP opcode is guarded; do not turn it into a gameplay patch.
        import struct
        gen.nso.image.extend(b'\x00' * (UI - gen.base + 4 - len(gen.nso.image)))
        struct.pack_into('<I', gen.nso.image, UI - gen.base, ts.UI_REFERENCE_SITES[UI][0])
        code = patches._stamp_trace_ui(gen, UI)
        self.assertIn('sw_stamp_followup_ui(', code)
        self.assertIn('sw_stamp_guest_site(', code)
        self.assertIn('c->x[1] = c->x[1];', code)
        for build in ('0' * 16, 'does-not-match', None):
            self.assertNotIn(UI, patches.table({}, build))
        struct.pack_into('<I', gen.nso.image, UI - gen.base, 0)
        with self.assertRaises(AssertionError):
            patches._stamp_trace_ui(gen, UI)

    def test_generator_instruments_selected_region(self):
        code = (ROOT/'tools/gen.py').read_text()
        self.assertIn('0x7000dd7000 <= entry < 0x7000e00000', code)
        self.assertIn('sw_stamp_followup_entry', code)
        self.assertIn('self.nso.build_id[:16]', code)

    def test_real_nso_ui_opcode_if_available(self):
        target = os.environ.get('SAVAGEWIND_TEST_MAIN_NSO')
        if not target:
            self.skipTest('SAVAGEWIND_TEST_MAIN_NSO not set')
        from nso import Nso
        n = Nso(target)
        self.assertEqual(n.build_id[:16], BUILD_ID)
        self.assertEqual(n.u32(UI - 0x7000000000), ts.UI_REFERENCE_SITES[UI][0])

    def test_readback_tracks_change_and_is_disabled_by_env(self):
        cc = shutil.which('clang') or shutil.which('gcc')
        if not cc:
            self.skipTest('No C compiler')
        source = r'''
#include <assert.h>
#include <stdint.h>
#include "gamelib.h"
const GameLibModule game_modules[1] = {0};
const uint32_t game_module_count = 0;
const char game_version[] = "1.9.0";
const GameLibSite game_sites[1] = {0};
const uint32_t game_site_count = 0;
extern const GameLibTables *swgame_tables(void);
extern void sw_stamp_capacity_stored(uint64_t,uint64_t,uint64_t,uint32_t,uint32_t,uint64_t);
extern void sw_stamp_followup_ui(uint64_t,const char *,const Cpu *);
extern void sw_stamp_followup_entry(uint64_t,const Cpu *);
int main(void) {
  uint8_t mem[1024] = {0};
  Cpu c = {0};
  uint64_t owner = (uint64_t)(uintptr_t)mem;
  uint64_t addr = owner + 0x228;
  assert(swgame_tables()->abi == GAMELIB_ABI);
  ST32(addr,9999); ST32(addr+4,5);
  sw_stamp_capacity_stored(0x7000d844f4,addr,((uint64_t)5<<32)|9999,9999,5,0);
  c.x[0]=0x42; c.x[30]=0x7000aa0000; c.nzcv=0xa0000000;
  sw_stamp_followup_ui(0x7000dd8208,"open_stamp_menu",&c);
  for (int i=0;i<129;i++) sw_stamp_followup_entry(0x7000dd863c,&c);
  ST32(addr,100);
  sw_stamp_followup_ui(0x7000dd86b4,"stamp_count_widget",&c);
  for (int i=0;i<129;i++) sw_stamp_followup_entry(0x7000dd863c,&c);
  assert(LD32(addr)==100 && LD32(addr+4)==5);
  assert(c.nzcv == 0xa0000000 && c.x[0]==0x42 && c.x[30]==0x7000aa0000);
  return 0;
}
'''
        with tempfile.TemporaryDirectory() as tmp:
            d = Path(tmp)
            src=d/'main.c'; src.write_text(source)
            exe=d/'harness'
            cmd=[cc,'-std=gnu11','-O1','-Wall','-Wextra','-Werror',
                 '-Iruntime','-DGAMELIB_ID="test"','tools/gamelib_entry.c',str(src),'-o',str(exe)]
            b=subprocess.run(cmd,cwd=ROOT,text=True,capture_output=True)
            self.assertEqual(b.returncode,0,b.stderr)
            env=os.environ.copy(); env.pop('SAVAGEWIND_STAMP_FOLLOWUP_TRACE',None)
            r=subprocess.run([str(exe)],cwd=d,env=env,text=True,capture_output=True)
            self.assertEqual(r.returncode,0,r.stderr)
            log=(d/'stamp-followup-v6.log').read_text()
            self.assertIn('v6 armed',log)
            self.assertRegex(log,r'stage=ui .*tag=open_stamp_menu .*readable=1 capacity=9999 pins=5')
            self.assertRegex(log,r'stage=ui .*tag=stamp_count_widget .*readable=1 capacity=100 pins=5')
            self.assertRegex(log,r'stage=fn .*readable=1 capacity=9999 pins=5')
            self.assertRegex(log,r'stage=fn .*readable=1 capacity=100 pins=5')
            self.assertIn('nzcv=0xa0000000', log)
            self.assertRegex(log,r'stage=fn .*hits=128 .*capacity=9999')
            self.assertRegex(log,r'stage=fn .*hits=256 .*capacity=100')
            (d/'stamp-followup-v6.log').unlink()
            env['SAVAGEWIND_STAMP_FOLLOWUP_TRACE']='0'
            r=subprocess.run([str(exe)],cwd=d,env=env,text=True,capture_output=True)
            self.assertEqual(r.returncode,0,r.stderr)
            self.assertFalse((d/'stamp-followup-v6.log').exists())

if __name__=='__main__':
    unittest.main()
