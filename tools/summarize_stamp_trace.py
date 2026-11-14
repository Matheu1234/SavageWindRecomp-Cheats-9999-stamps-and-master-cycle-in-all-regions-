#!/usr/bin/env python3
"""Summarize non-mutating stamp trace logs from separate gameplay sessions."""
import argparse
import collections
from pathlib import Path
import re

RX = re.compile(r'\[stamp-trace\] type=(cmp|mov|ui) pc=(0x[0-9a-f]+) (?:value=(\d+) imm=(\d+)|event=([^ ]+)) hits=(\d+)')

def extract(path):
    events=[]
    for line in Path(path).read_text(encoding='utf-8',errors='replace').splitlines():
        m=RX.search(line)
        if m:
            kind,pc,val,imm,event,hits=m.groups()
            events.append((kind,int(pc,16), int(val) if val else None,
                           int(imm) if imm else None,event,int(hits)))
    return events

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('logs',nargs='+')
    args=ap.parse_args()
    for name in args.logs:
        evs=extract(name)
        print(f'=== {name}: {len(evs)} sampled events ===')
        for kind in ('ui','cmp','mov'):
            group=collections.defaultdict(list)
            for event in evs:
                if event[0] == kind:
                    group[event[1]].append(event)
            print(f'{kind.upper()}: {len(group)} unique sites')
            for pc,events in sorted(group.items()):
                vals=sorted(set(e[2] for e in events if e[2] is not None))
                names=sorted(set(e[4] for e in events if e[4]))
                print(f'  {pc:#x} n={len(events)} hits={max(e[5] for e in events)}'
                      f' values={vals[:15]} events={names}')

if __name__=='__main__': main()
