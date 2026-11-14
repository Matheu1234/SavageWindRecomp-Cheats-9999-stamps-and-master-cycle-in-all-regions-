"""Static tests of the opt-in 1.9.0 Master Cycle mask hook. No game required."""
from pathlib import Path
import sys
import unittest

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
import patches

class NsoStub:
    def __init__(self, build='cd57b23fa4bbad65', lo=0x5293fbe8, hi=0x72a000e8):
        self.build_id=build
        self.image=bytearray(0xb37124)
        self.image[0xb3711c:0xb37120]=lo.to_bytes(4,'little')
        self.image[0xb37120:0xb37124]=hi.to_bytes(4,'little')

class GenStub:
    base=0x7000000000
    def __init__(self, **kw):self.nso=NsoStub(**kw)

class TestMotorcyclePatch(unittest.TestCase):
    def test_known_build_only(self):
        self.assertIn(0x7000b37120,patches.table({'anything':3}, 'cd57b23fa4bbad65'))
        self.assertNotIn(0x7000b37120,patches.table({'anything':3}, 'de0b6ac4efc1dda7'))
        self.assertNotIn(0x7000b37120,patches.table({'anything':3}))

    def test_original_off_and_all_climates_on(self):
        cb=patches.table({},'cd57b23fa4bbad65')[0x7000b37120]
        result=cb(GenStub(),0x7000b37120)
        self.assertIn('0x79fdfull',result)
        self.assertIn('0xfffffull',result)
        self.assertIn('rt_sw_motorcycle_all_regions()',result)
        self.assertEqual([i for i in range(20) if not 0x79fdf & (1<<i)], [5,13,14,19])
        self.assertTrue(all(0xfffff & (1<<i) for i in range(20)))

    def test_fail_closed_on_wrong_instruction(self):
        cb=patches.table({},'cd57b23fa4bbad65')[0x7000b37120]
        with self.assertRaises(AssertionError): cb(GenStub(lo=0),0x7000b37120)
        with self.assertRaises(AssertionError): cb(GenStub(hi=0),0x7000b37120)
        with self.assertRaises(AssertionError): cb(GenStub(build='de0b6ac4efc1dda7'),0x7000b37120)

    def test_address_collision_rejected(self):
        saved=patches.PATCHES.copy()
        try:
            patches.PATCHES['inject_collision']=lambda gen,a:'bad'
            with self.assertRaises(AssertionError):patches.table({'inject_collision':0x7000b37120}, 'cd57b23fa4bbad65')
        finally:
            patches.PATCHES.clear();patches.PATCHES.update(saved)

if __name__=='__main__': unittest.main()
