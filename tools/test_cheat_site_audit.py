"""Tests for the read-only Switch cheat-site triage helpers."""
import struct
import unittest
from cheat_site_audit import adrp_add_xrefs, adrp_page, movz_w_imm, cmp_w_imm


class AuditTests(unittest.TestCase):
    def test_adrp_page(self):
        self.assertEqual(adrp_page(0x90000003, 0x1200), 0x1000)
        self.assertIsNone(adrp_page(0x52800c80, 0x1200))

    def test_direct_string_reference(self):
        # ADRP X3, data page at +0x2000; ADD X3, X3, #0xabc
        image=bytearray(0x4000)
        struct.pack_into('<I',image,0x20,0x90000003 | (2<<29))
        # Above encodes ADRP with immlo=2 -> +0x2000 relative to page 0.
        struct.pack_into('<I',image,0x24,0x91000000 | (0xabc<<10) | (3<<5) | 3)
        found=adrp_add_xrefs(image,0x200,{0x2abc})
        self.assertEqual(found[0x2abc],[(0x20,0x24)])

    def test_ignore_other_references(self):
        image=bytearray(0x4000)
        struct.pack_into('<I',image,0x20,0x90000003 | (2<<29))
        struct.pack_into('<I',image,0x24,0x91000000 | (0xabc<<10) | (4<<5) | 3)
        self.assertEqual(adrp_add_xrefs(image,0x200,{0x2abc}),{})

    def test_immediates(self):
        self.assertTrue(movz_w_imm(0x52800c80,100))
        self.assertTrue(movz_w_imm(0x52800c89,100))
        self.assertFalse(movz_w_imm(0x52800c80,200))
        self.assertTrue(cmp_w_imm(0x7101901f,100))
        self.assertTrue(cmp_w_imm(0x7101905f,100))
        self.assertFalse(cmp_w_imm(0x7101901f,200))

if __name__=='__main__':
    unittest.main()
