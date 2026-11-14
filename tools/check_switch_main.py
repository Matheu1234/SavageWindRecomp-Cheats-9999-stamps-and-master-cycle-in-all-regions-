#!/usr/bin/env python3
"""Read BOTW Switch NSO metadata locally; never copy or modify the game file.

Run from the Savage Wind source folder:
    python tools/check_switch_main.py /path/to/extracted/exefs/main

This is a header-only metadata check; hook opcodes are checked separately by patches.py.
"""
import argparse
import json
from pathlib import Path
import nso

SUPPORTED = {'cd57b23fa4bbad65': '1.9.0', 'de0b6ac4efc1dda7': '1.8.2'}
KEYWORDS = ('stamp', 'pin', 'marker', 'motorcycle', 'mastercycle', 'master_cycle', 'bike', 'horse')


def inspect(path, show_symbols=False):
    info = nso.header(path)
    bid = info['build_id']
    version = SUPPORTED.get(bid[:16], 'unknown')
    ret={'file':Path(path).name, 'format':'NSO0', 'build_id':bid[:16], 'known_version':version,
         'recommended_port_target':version == '1.9.0',
         'cheat_patch_locations':'not identified', 'source_file_modified':False}
    if show_symbols:
        module=nso.Nso(path)
        found=sorted({name for name, _, _, _, _ in module.exports() if any(k in name.lower() for k in KEYWORDS)})
        ret['candidate_exported_symbols']=found[:300]
        ret['symbol_candidates_are_proof_of_patch_site']=False
    return ret


def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('main_nso',help='local BOTW Switch ExeFS main file')
    ap.add_argument('--symbols', action='store_true',help='inspect exported symbol names (may be empty or inconclusive)')
    args=ap.parse_args()
    print(json.dumps(inspect(args.main_nso, args.symbols), ensure_ascii=False, indent=2))

if __name__=='__main__': main()
