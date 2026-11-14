"""Unit tests for the Cemu patch parser and Switch metadata inspection utility.

No copyrighted game files or native ARM64 hook sites are needed.
"""
import importlib.util
from pathlib import Path
import tempfile
import shutil
import struct
import sys
import unittest

ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'tools'))
import check_cemu_packs as p
import check_switch_main as s

class ReferencePatches(unittest.TestCase):
    def test_exact_sources(self):
        got=p.audit()
        self.assertEqual(set(got),{'9999StampsLimit','MotorcycleAllRegions'})
        self.assertEqual(got['9999StampsLimit']['instruction'],'li r0, 9999')
        self.assertEqual(got['MotorcycleAllRegions']['instruction'],'li r3, 1')
        self.assertEqual(got['9999StampsLimit']['switch_arm64_site'],'0x7000D844E0')
        self.assertEqual(got['MotorcycleAllRegions']['switch_arm64_site'],'0x7000B37120')
        self.assertTrue(all('experimental handler' in item['status'] for item in got.values()))
    def test_bad_value_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            d=Path(tmp)
            shutil.copytree(p.BASE,d/'packs')
            asm=d/'packs/9999StampsLimit/patch_9999StampsLimit.asm'
            asm.write_text(asm.read_text().replace('0x0270f','0x0270e'))
            with self.assertRaises(ValueError): p.audit(d/'packs')
    def test_hash_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            d=Path(tmp)
            shutil.copytree(p.BASE,d/'packs')
            asm=d/'packs/MotorcycleAllRegions/patch_MotorcycleAllRegions.asm'
            asm.write_text(asm.read_text().replace('0x6267BFD0','0x6267BFD1'))
            with self.assertRaises(ValueError): p.audit(d/'packs')

class SwitchMetadata(unittest.TestCase):
    def test_header_only_does_not_read_game_code(self):
        with tempfile.TemporaryDirectory() as tmp:
            f=Path(tmp)/'main'
            buf=bytearray(0x100)
            buf[:4]=b'NSO0'
            buf[0x40:0x48]=bytes.fromhex('cd57b23fa4bbad65')
            f.write_bytes(buf)
            info=s.inspect(str(f))
            self.assertEqual(info['known_version'],'1.9.0')
            self.assertTrue(info['recommended_port_target'])
            self.assertEqual(info['cheat_patch_locations'],'not identified')
    def test_bad_nso_rejected(self):
        with tempfile.TemporaryDirectory() as tmp:
            f=Path(tmp)/'main'; f.write_bytes(b'not-an-nso')
            with self.assertRaises(AssertionError): s.inspect(str(f))

if __name__=='__main__': unittest.main()
