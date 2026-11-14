#!/usr/bin/env python3
"""Summarize one read-only guest-code stamp probe log from BOTW 1.9.0.

Usage: python tools/analyze_stamp_guest_v3.py stamp-guest-v3.log
A ranking is only a hypothesis, never proof that a PC is the stamp limit.
"""
from collections import defaultdict
from pathlib import Path
import argparse
import re

KV = re.compile(r'([a-z][a-z0-9_]*)=([^\s]+)')
TYPES = {'ui', 'cmp', 'mov', 'fn'}

def parse(text):
    for line in text.splitlines():
        if not line.startswith('[stamp-guest] seq='):
            continue
        kv = dict(KV.findall(line))
        if kv.get('type') not in TYPES or 'pc' not in kv:
            continue
        try:
            yield {'seq': int(kv['seq']), 'epoch': int(kv['epoch']),
                   'type': kv['type'], 'pc': int(kv['pc'], 0),
                   'hits': int(kv['hits']),
                   'operand': int(kv['operand'], 0) if 'operand' in kv else None,
                   'imm': int(kv['imm']) if 'imm' in kv else None,
                   'lr': int(kv['lr'], 0) if 'lr' in kv else None,
                   'x1': int(kv['x1'], 0) if 'x1' in kv else None}
        except (KeyError, ValueError):
            continue

def summarize(events):
    groups = defaultdict(list)
    for event in events:
        groups[event['type'], event['pc']].append(event)
    rows = []
    for (kind, pc), group in groups.items():
        epochs = len(set(e['epoch'] for e in group if e['epoch']))
        operands = set(e['operand'] for e in group if e['operand'] is not None)
        x1s = set(e['x1'] for e in group if e['x1'] is not None)
        rows.append({'type':kind, 'pc':pc, 'records':len(group), 'epochs':epochs,
                     'distinct_operands':len(operands), 'distinct_x1':len(x1s),
                     'max_hits':max(e['hits'] for e in group),
                     'samples': sorted(operands)[:12]})
    # Favor changing operands and event-related calls, but do not assert their roles.
    rows.sort(key=lambda r: (r['type']=='ui', r['distinct_operands'] > 1,
                             r['distinct_x1'] > 1, r['epochs'], r['records']), reverse=True)
    return rows

def render(rows, events):
    lines = ['# Stamp Guest v3 — resumo das observações', '',
             f'- Registros: **{len(events)}**',
             f'- Endereços: **{len(rows)}**',
             f'- Aberturas observadas do menu: **{max((e["epoch"] for e in events),default=0)}**',
             '', 'Esses indicadores não demonstram, isoladamente, o limite de 100 stamps.', '',
             '| Tipo | PC ARM64 | Registros | Épocas | Valores distintos | X1 distintos | Exemplos do operando |',
             '|---|---|---:|---:|---:|---:|---|']
    for r in rows[:90]:
        vals=', '.join(str(a) for a in r['samples']) if r['samples'] else '—'
        lines.append(f'| {r["type"]} | `0x{r["pc"]:x}` | {r["records"]} | {r["epochs"]} | {r["distinct_operands"]} | {r["distinct_x1"]} | {vals} |')
    return '\n'.join(lines)+'\n'

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('log',type=Path)
    ap.add_argument('--out',type=Path)
    args=ap.parse_args()
    data=list(parse(args.log.read_text(encoding='utf-8',errors='replace')))
    result=render(summarize(data),data)
    if args.out:
        args.out.write_text(result, encoding='utf-8')
        print(f'Created {args.out}')
    else:
        print(result)
    if not data:
        raise SystemExit('No stamp-guest events; ensure the updated swgame.dll and variable are active.')
if __name__=='__main__': main()
