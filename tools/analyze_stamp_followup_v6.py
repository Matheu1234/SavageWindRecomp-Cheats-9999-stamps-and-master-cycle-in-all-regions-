#!/usr/bin/env python3
"""Summarize a read-only v6 stamp trace; does not infer function semantics from registers."""
from collections import Counter, defaultdict
from pathlib import Path
import argparse
import re

KV = re.compile(r'(\w+)=([^\s]+)')

def report(path):
    rows = []
    for line in path.read_text(encoding='utf-8', errors='replace').splitlines():
        if not line.startswith('[stamp-followup] seq='):
            continue
        rows.append(dict(KV.findall(line)))
    ui = [row for row in rows if row.get('stage') == 'ui']
    fn = [row for row in rows if row.get('stage') == 'fn']
    reads = [r for r in rows if r.get('readable') == '1']
    counts = Counter(int(r['capacity']) for r in reads if 'capacity' in r)
    pcs = defaultdict(list)
    for r in fn:
        pcs[r.get('pc','?')].append(r)
    lines = [f'# v6 stamp follow-up: {path.name}', '',
             f'- Eventos: {len(rows)} ({len(ui)} UI, {len(fn)} entradas de função)',
             f'- Leituras válidas: {len(reads)}',
             f'- Valores de capacidade observados: {dict(sorted(counts.items()))}',
             f'- Amostras sem leitura: {len(rows)-len(reads)}',
             f'- Endereços de função distintos: {len(pcs)}',
             '- Estes são dados observados, não uma identificação automática da rotina de inserção.',
             '', '## Eventos de interface', '']
    for r in ui[:100]:
        lines.append(f"- seq={r.get('seq')} epoch={r.get('epoch')} pc={r.get('pc')} "
                     f"tag={r.get('tag')} capacity={r.get('capacity')} readable={r.get('readable')}")
    lines += ['', '## Funções amostradas (maior quantidade de registros)', '',
              '| PC | Amostras | Máximo hits | LR distintos |', '|---|---:|---:|---:|']
    for pc, events in sorted(pcs.items(), key=lambda kv: (-len(kv[1]), kv[0]))[:100]:
        mx = max((int(e.get('hits','0')) for e in events), default=0)
        lines.append(f'| `{pc}` | {len(events)} | {mx} | {len(set(e.get("lr") for e in events))} |')
    if rows and len(rows) >= 12000:
        lines += ['', '**Atenção:** possivelmente atingiu o limite global de amostras.']
    return '\n'.join(lines) + '\n'

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('trace',type=Path);p.add_argument('--out',type=Path)
    a=p.parse_args();result=report(a.trace)
    if a.out:a.out.write_text(result,encoding='utf-8');print(f'Created {a.out}')
    else:print(result)
