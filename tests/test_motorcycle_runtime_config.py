"""Host-only tests: Savage Wind motorcycle flag defaults off, parses INI and fails closed."""
import ctypes
import os
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

ROOT=Path(__file__).resolve().parents[1]

@unittest.skipUnless(shutil.which('clang') and os.name == 'posix', 'requires clang on POSIX')
class MotorcycleConfig(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp=tempfile.TemporaryDirectory()
        obj=Path(cls.tmp.name)/'cheats.so'
        subprocess.run(['clang','-std=c11','-Wall','-Wextra','-Werror','-fPIC','-shared',
                        str(ROOT/'runtime/sw_cheats.c'),'-o',str(obj)],check=True)
        cls.lib=ctypes.CDLL(str(obj))
        cls.lib.rt_sw_motorcycle_all_regions.restype=ctypes.c_int

    @classmethod
    def tearDownClass(cls): cls.tmp.cleanup()

    def test_default_on_and_invalid_off(self):
        with tempfile.TemporaryDirectory() as d:
            pwd=os.getcwd()
            try:
                os.chdir(d)
                shutil.copytree(ROOT/'cheats',Path(d)/'cheats')
                self.lib.rt_sw_cheat_init()
                self.assertEqual(self.lib.rt_sw_motorcycle_all_regions(),0)
                Path('savagewind-cheats.ini').write_text('[cheats]\nmotorcycle_all_regions = 1\n')
                self.lib.rt_sw_cheat_init()
                self.assertEqual(self.lib.rt_sw_motorcycle_all_regions(),1)
                Path('savagewind-cheats.ini').write_text('[cheats]\nmotorcycle_all_regions = invalid\n')
                self.lib.rt_sw_cheat_init()
                self.assertEqual(self.lib.rt_sw_motorcycle_all_regions(),0)
                Path('savagewind-cheats.ini').write_text('[cheats]\nmotorcycle_all_regions = 0\n')
                self.lib.rt_sw_cheat_init()
                self.assertEqual(self.lib.rt_sw_motorcycle_all_regions(),0)
            finally:
                os.chdir(pwd)

if __name__ == '__main__': unittest.main()
