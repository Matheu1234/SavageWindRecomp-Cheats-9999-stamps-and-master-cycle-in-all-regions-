"""The v3 guest probe is entirely inside the game code library (no runtime phase switch)."""
from pathlib import Path
import os
import re
import shutil
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parent.parent

class StampGuestProbeTests(unittest.TestCase):
    def build(self, target):
        cc = shutil.which('clang') or shutil.which('gcc')
        if not cc:
            self.skipTest('No compiler available')
        cmd = [cc, '-std=gnu11', '-O1', '-Wall', '-Wextra', '-Werror',
               '-Iruntime', '-DGAMELIB_ID="test"',
               'tools/gamelib_entry.c', 'tests/stamp_guest_probe_harness.c', '-o', str(target)]
        subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True, check=True)

    def test_probe_disabled_by_default(self):
        with tempfile.TemporaryDirectory() as d:
            binary=Path(d)/'harness';self.build(binary)
            env=dict(os.environ);env.pop('SAVAGEWIND_STAMP_GUEST_TRACE', None)
            output=subprocess.run([str(binary)],env=env, cwd=d, capture_output=True,text=True,check=True)
            self.assertNotIn('[stamp-guest]',output.stderr)
            self.assertFalse((Path(d)/"stamp-guest-v3.log").exists())

    def test_probe_generated_in_game_library(self):
        with tempfile.TemporaryDirectory() as d:
            binary=Path(d)/'harness';self.build(binary)
            env=dict(os.environ);env['SAVAGEWIND_STAMP_GUEST_TRACE']='1'
            output=subprocess.run([str(binary)],env=env,cwd=d,capture_output=True,text=True,check=True)
            self.assertIn("output=stamp-guest-v3.log", output.stderr)
            lines=[a for a in (Path(d)/"stamp-guest-v3.log").read_text().splitlines() if '[stamp-guest]' in a]
            self.assertIn('enabled v3', lines[0])
            self.assertTrue(any('type=ui' in a and 'epoch=1' in a for a in lines))
            self.assertTrue(any('type=cmp' in a and 'operand=42' in a for a in lines))
            self.assertTrue(any('type=cmp' in a and 'operand=43' in a for a in lines))
            self.assertTrue(any('type=fn' in a and 'x1=0x5' in a for a in lines))
            self.assertTrue(any('type=ui' in a and 'epoch=2' in a for a in lines))
            self.assertFalse(any('epoch=0 type=cmp' in a for a in lines))
            numbers=[int(m.group(1)) for line in lines for m in [re.search(r'seq=(\d+)',line)] if m]
            self.assertEqual(numbers,sorted(numbers))

    def test_codegen_is_build_and_opcode_guarded(self):
        code=(ROOT/'tools/patches.py').read_text()
        gen=(ROOT/'tools/gen.py').read_text()
        entry=(ROOT/'tools/gamelib_entry.c').read_text()
        self.assertIn('sw_stamp_guest_site',code)
        self.assertIn('original.code',code)
        self.assertIn('assert w == ts.CMP_SITES[a]',code)
        self.assertIn('self.nso.build_id[:16]',gen)
        self.assertIn('sw_stamp_guest_entry',gen)
        self.assertIn('SAVAGEWIND_STAMP_GUEST_TRACE',entry)
        self.assertNotIn('stamp-flow-control.txt',entry)
        self.assertNotIn('rt_sw_stamp_flow',entry)
        self.assertNotIn('rt_sw_stamp_trace_record',entry)

if __name__=='__main__':unittest.main()
