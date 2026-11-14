#!/usr/bin/env python3
"""Read-only, build-ID-locked triage for two Cemu cheats on Switch BOTW.

This deliberately DOES NOT patch the game or claim that a constant/string
reference identifies a game function. It gives reproducible starting points
for manual call-graph analysis and separate save-memory verification.

Usage: python tools/cheat_site_audit.py /path/to/exefs/main --json
"""
import argparse
from collections import defaultdict
import json
import struct
from pathlib import Path

from nso import Nso, header

EXPECTED_BUILD_ID = 'cd57b23fa4bbad65'  # Savage Wind Switch v1.9.0
MAIN_BASE = 0x7000000000
CONTEXT_STRINGS = (
    'mc_openStampMenu', 'Pa_StampNum_00', 'Pa_StampBox_00',
    'BikeAppearPointDecide', 'BikeAppearPointPreview',
    'IsGet_Obj_Motorcycle', 'Motorcycle', 'Motorcycle_Energy',
)


def signed21(value):
    return value - (1 << 21) if value & (1 << 20) else value


def adrp_page(word, instruction_offset):
    """Return module-relative page referenced by an ADRP instruction."""
    if (word & 0x9f000000) != 0x90000000:
        return None
    val = (((word >> 5) & 0x7ffff) << 2) | ((word >> 29) & 3)
    return (instruction_offset & ~4095) + signed21(val) * 4096


def adrp_add_xrefs(image, code_size, targets, window_instructions=8):
    """Find literal ADRP + ADD Xbase, Xbase, #imm materializations.

    These are candidate references only: register reuse between the two
    instructions and references via GOT, pointer tables or ADR are not resolved.
    """
    pages = defaultdict(set)
    for target in targets:
        pages[target & ~4095].add(target)
    found = defaultdict(list)
    for pc in range(0, code_size - 4, 4):
        word = struct.unpack_from('<I', image, pc)[0]
        page = adrp_page(word, pc)
        if page not in pages:
            continue
        base_reg = word & 31
        for distance in range(1, window_instructions + 1):
            add_pc = pc + 4 * distance
            if add_pc + 4 > code_size:
                break
            candidate = struct.unpack_from('<I', image, add_pc)[0]
            if (candidate & 0xff800000) != 0x91000000:  # ADD Xd, Xn, #imm12, shift 0
                continue
            if ((candidate >> 5) & 31) != base_reg:
                continue
            offset = (candidate >> 10) & 0xfff
            resolved = page + offset
            if resolved in pages[page]:
                found[resolved].append((pc, add_pc))
                break
    return found


def movz_w_imm(word, imm):
    """MOVZ Wd, #imm (not a semantic proof of a stamp limit)."""
    return (word & 0xffffffe0) == (0x52800000 | (imm << 5))


def cmp_w_imm(word, imm):
    """SUBS WZR, Wn, #imm, lsl #0 (CMP Wn, #imm)."""
    return (word & 0xffc0001f) == 0x7100001f and ((word >> 10) & 0xfff) == imm


def inspect(main_file):
    h = header(main_file)
    if h['build_id'][:16] != EXPECTED_BUILD_ID:
        raise ValueError(f'unsupported build ID {h["build_id"][:16]}, expected {EXPECTED_BUILD_ID}')
    nso = Nso(main_file)
    image = nso.image
    text_offset, text_size = nso.text
    if text_offset != 0:
        raise ValueError('unexpected code segment offset')
    code = memoryview(image)[text_offset:text_offset + text_size]
    mov_100 = []
    cmp_100 = []
    for pc in range(0, len(code) - 3, 4):
        word = struct.unpack_from('<I', code, pc)[0]
        if movz_w_imm(word, 100):
            mov_100.append({'address':hex(MAIN_BASE+pc),'register':'w'+str(word & 31)})
        if cmp_w_imm(word, 100):
            cmp_100.append({'address':hex(MAIN_BASE+pc),'register':'w'+str((word >> 5)&31)})
    targets = {}
    ro_begin, ro_size = nso.rodata
    for term in CONTEXT_STRINGS:
        needle = term.encode('utf-8') + b'\0'
        at = image.find(needle, ro_begin, ro_begin + ro_size)
        while at >= 0 and at > ro_begin and image[at - 1] != 0:
            at = image.find(needle, at + 1, ro_begin + ro_size)
        if at >= 0:
            targets[term] = at
    crossrefs = adrp_add_xrefs(image, text_size, targets.values())
    out = {
        'source_filename':Path(main_file).name,
        'source_build_id':h['build_id'][:16],
        'game_version':'Switch BOTW 1.9.0',
        'read_only':True,
        'patch_site_verified':False,
        'cheats_enabled':False,
        'status':'Investigative clues only; none of these is an approved patch address.',
        'mov_w_100_count':len(mov_100),
        'mov_w0_100_candidates':[m['address'] for m in mov_100 if m['register']=='w0'],
        'cmp_w_100_count':len(cmp_100),
        'string_references':[
            {
                'string':term,
                'data_address':hex(MAIN_BASE+at),
                'possible_code_references':[hex(MAIN_BASE+pc) for pc,_ in crossrefs.get(at,[])],
            }
            for term,at in targets.items()
        ],
        'required_next_checks':[
            'Trace the map stamp UI methods to the actual count guard and save serializer.',
            'Verify backing capacity and pin+stamp counts before considering a 9999 limit.',
            'Trace motorcycle summon eligibility separately from unlock and DLC prerequisites.',
            'Validate exact instructions on the pinned game build and require unique pattern matches.',
            'Run in-game and save round-trip tests before enabling either cheat.',
        ]
    }
    return out


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument('main_nso',help='BOTW Switch 1.9.0 original main NSO')
    ap.add_argument('--json',action='store_true',help='print machine-readable report')
    args=ap.parse_args()
    result=inspect(args.main_nso)
    if args.json:
        print(json.dumps(result,ensure_ascii=False,indent=2))
    else:
        print(f"BOTW Switch {result['game_version']}; ID {result['source_build_id']}")
        print(f"MOV W*, #100: {result['mov_w_100_count']} (W0: {len(result['mov_w0_100_candidates'])})")
        print(f"CMP W*, #100: {result['cmp_w_100_count']}")
        for ref in result['string_references']:
            print(f"{ref['string']}: {', '.join(ref['possible_code_references']) or '[no direct xref]'}")
        print(result['status'])


if __name__=='__main__':
    main()
