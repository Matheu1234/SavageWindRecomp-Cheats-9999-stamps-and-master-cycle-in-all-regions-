#!/usr/bin/env python3
"""Audit original Cemu PowerPC cheat packs against expected Wii U instructions.

This verification is informational; Switch v4 hooks live in tools/patches.py.
"""
import argparse
import json
import re
from pathlib import Path

BASE = Path(__file__).resolve().parent.parent / 'reference' / 'cemu_graphic_packs'
SPECS = {
  '9999StampsLimit': {'file':'patch_9999StampsLimit.asm', 'offset':0x2E9F2C0, 'register':'r0', 'value':9999},
  'MotorcycleAllRegions': {'file':'patch_MotorcycleAllRegions.asm', 'offset':0x02A32A30, 'register':'r3', 'value':1},
}
EXPECTED_MODULE=0x6267BFD0


def audit(root=BASE):
    output = {}
    for name, want in SPECS.items():
        folder = root / name
        text = (folder / want['file']).read_text(encoding='utf-8')
        rules = (folder / 'rules.txt').read_text(encoding='utf-8')
        match = re.search(r'(?m)^\s*moduleMatches\s*=\s*0x([0-9a-fA-F]+)',text)
        if not match or int(match.group(1),16)!=EXPECTED_MODULE:
            raise ValueError(f'{name}: unsupported/mismatched Wii U module hash')
        candidates = re.findall(r'(?m)^\s*0x([0-9a-fA-F]+)\s*=\s*li\s+(r\d+)\s*,\s*(0x[0-9a-fA-F]+|\d+)',text)
        if len(candidates)!=1:
            raise ValueError(f'{name}: expected exactly one Wii U instruction patch, got {len(candidates)}')
        address,reg,value=candidates[0]
        if (int(address,16),reg,int(value,0))!=(want['offset'],want['register'],want['value']):
            raise ValueError(f'{name}: unexpected PowerPC patch contents')
        title_ids=re.search(r'(?m)^\s*titleIds\s*=\s*(.+)$',rules)
        if not title_ids or '00050000101C9400' not in title_ids.group(1):
            raise ValueError(f'{name}: not a matching Cemu BOTW rules.txt')
        output[name]={'platform':'Wii U / PowerPC', 'moduleMatches':f'{EXPECTED_MODULE:08X}',
                      'instruction':f'li {reg}, {want["value"]}', 'wiiu_offset':f'0x{want["offset"]:08X}',
                      'switch_arm64_site': ('0x7000D844E0' if name=='9999StampsLimit' else '0x7000B37120'),
                      'status':'experimental handler (1.9.0 only); 9999 save serialization NOT expanded'}
    return output


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--verify', action='store_true',help='assert both known Wii U patches match supplied references')
    args=ap.parse_args()
    result=audit()
    if args.verify: print('OK: both Wii U reference patches match the expected module hash and instruction values.')
    else: print(json.dumps(result, indent=2, ensure_ascii=False))

if __name__=='__main__': main()
