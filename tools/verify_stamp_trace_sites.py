#!/usr/bin/env python3
"""Verify every diagnostic site opcode and build ID in the original NSO; read-only."""
import argparse
import json
from nso import Nso, header
from stamp_trace_sites import BUILD_ID, CMP_SITES, MOV_W0_100_SITES, UI_REFERENCE_SITES
import struct
BASE=0x7000000000

def inspect(path):
    bid=header(path)['build_id'][:16]
    if bid != BUILD_ID: raise ValueError(f'unsupported build {bid}')
    nso=Nso(path)
    failed=[]
    for group in (CMP_SITES, MOV_W0_100_SITES, UI_REFERENCE_SITES):
        for a,word in group.items():
            expected = word[0] if isinstance(word,tuple) else word
            actual=struct.unpack_from('<I',nso.image,a-BASE)[0]
            if actual != expected: failed.append({'address':hex(a),'expected':hex(expected),'actual':hex(actual)})
    result={'build_id':bid,'checked':len(CMP_SITES)+len(MOV_W0_100_SITES)+len(UI_REFERENCE_SITES),
            'cmp_sites':len(CMP_SITES),'mov_w0_100_sites':len(MOV_W0_100_SITES),
            'ui_sites':len(UI_REFERENCE_SITES),'failed':failed,'read_only':True}
    return result

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('main_nso');ap.add_argument('--json',action='store_true')
    a=ap.parse_args();res=inspect(a.main_nso)
    print(json.dumps(res,indent=2) if a.json else str(res))
    if res['failed']:raise SystemExit(1)
if __name__=='__main__':main()
