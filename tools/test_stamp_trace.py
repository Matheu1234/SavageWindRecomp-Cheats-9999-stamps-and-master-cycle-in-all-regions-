"""Unit checks of diagnostic hooks: original ARM64 instruction is always emitted."""
import unittest
from types import SimpleNamespace
from lifter import Lifter
import patches
import stamp_trace_sites as t

class Gen:
    def __init__(self,word):
        self.nso=SimpleNamespace(build_id=t.BUILD_ID,image=None)
        self.base=0x7000000000
        self.lifter=Lifter(self.base,simd='c')
        self._word=word

def simulate(group,fn):
    for pc,expected in group.items():
        word=expected[0] if isinstance(expected,tuple) else expected
        gen=Gen(word)
        def fake(_,addr):
            if addr != pc:raise AssertionError('wrong address')
            return word
        before=patches._word
        patches._word=fake
        try:
            original=gen.lifter.lift(word,pc)
            assert original.kind=='op'
            patched=fn(gen,pc)
            assert original.code in patched
            assert 'rt_sw_stamp_trace_enabled' in patched
        finally:
            patches._word=before

class TraceTests(unittest.TestCase):
    def test_cmp_semantics_preserved(self):simulate(t.CMP_SITES,patches._stamp_trace_cmp)
    def test_mov_semantics_preserved(self):simulate(t.MOV_W0_100_SITES,patches._stamp_trace_mov)
    def test_ui_semantics_preserved(self):simulate(t.UI_REFERENCE_SITES,patches._stamp_trace_ui)
    def test_no_collision_with_motorcycle(self):
        addresses=set(t.CMP_SITES)|set(t.MOV_W0_100_SITES)|set(t.UI_REFERENCE_SITES)
        self.assertEqual(len(addresses),len(t.CMP_SITES)+len(t.MOV_W0_100_SITES)+len(t.UI_REFERENCE_SITES))
        self.assertNotIn(0x7000b37120,addresses)
    def test_malformed_build_is_not_instrumented(self):
        self.assertFalse(any(a in patches.table({},'de0b6ac4efc1dda7') for a in t.CMP_SITES))

if __name__=='__main__':unittest.main()
