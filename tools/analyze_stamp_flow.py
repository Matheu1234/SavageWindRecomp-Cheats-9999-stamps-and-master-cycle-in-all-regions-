#!/usr/bin/env python3
"""Compare labelled BOTW function-entry samples, not guesses about stamp limits.

Usage: python tools/analyze_stamp_flow.py baseline.tsv add.tsv remove.tsv
A site exclusive to an action is a candidate, NOT proof of a marker action.
Counters are compared as rates per million sampled entries because phase
length and background activity may differ.
"""
import argparse
from collections import Counter
import csv
from pathlib import Path

REQUIRED = {"pc", "lr", "hits", "x0", "x1", "x2", "x3", "x4", "x5"}


def load(path):
    path = Path(path)
    with path.open(encoding="utf-8", newline="") as f:
        lines = (line for line in f if not line.startswith("#"))
        reader = csv.DictReader(lines, delimiter="\t")
        if not reader.fieldnames or not REQUIRED.issubset(reader.fieldnames):
            raise ValueError(f"{path}: not a stamp-flow v2 TSV file")
        counts = Counter()
        samples = {}
        for row in reader:
            key = (int(row["pc"], 16), int(row["lr"], 16))
            count = int(row["hits"])
            if count < 1:
                raise ValueError(f"{path}: invalid hit count for {key!r}")
            counts[key] += count
            samples.setdefault(key, tuple(int(row[f"x{i}"], 16) for i in range(6)))
    return counts, samples


def rate(counter, key):
    total = sum(counter.values())
    return 1_000_000 * counter[key] / total if total else 0.0


def rank(baseline, action, top):
    exclusive = [k for k in action if k not in baseline]
    exclusive.sort(key=lambda k: (-action[k], k))
    lifted = [k for k in action if k in baseline]
    lifted.sort(key=lambda k: (-(rate(action, k) - rate(baseline, k)), -action[k], k))
    return exclusive[:top], [k for k in lifted if rate(action, k) > rate(baseline, k)][:top]


def show(label, baseline, action, top):
    exclusive, lifted = rank(baseline, action, top)
    print(f"\n=== {label}: {len(action)} observed caller edges, "
          f"{sum(action.values())} function calls ===")
    print(f"Only in {label}: {len([k for k in action if k not in baseline])} "
          "edges (candidates, not proven actions)")
    for pc, lr in exclusive:
        k = (pc, lr)
        print(f"  function=0x{pc:x} caller_lr=0x{lr:x} hits={action[k]}")
    print(f"Higher rate in {label}: (per million guest-function entries)")
    for pc, lr in lifted:
        k = (pc, lr)
        print(f"  function=0x{pc:x} caller_lr=0x{lr:x} "
              f"{label}={action[k]} baseline={baseline[k]} "
              f"rate={rate(action, k):.1f}/{rate(baseline, k):.1f}")


def main(argv=None):
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("baseline", help="TSV captured while using the menu without changes")
    ap.add_argument("add", help="TSV captured around a deliberate stamp insertion")
    ap.add_argument("remove", help="TSV captured around a deliberate stamp removal")
    ap.add_argument("--top", type=int, default=25)
    args = ap.parse_args(argv)
    if not 1 <= args.top <= 500:
        ap.error("--top must be between 1 and 500")
    baseline, _ = load(args.baseline)
    add, _ = load(args.add)
    remove, _ = load(args.remove)
    print(f"Baseline: {len(baseline)} edges, {sum(baseline.values())} total calls")
    show("add", baseline, add, args.top)
    show("remove", baseline, remove, args.top)
    common = set(add) & set(remove) - set(baseline)
    print("\nShared by add/remove, absent from baseline:", len(common))
    for pc, lr in sorted(common, key=lambda k: -(add[k] + remove[k]))[:args.top]:
        print(f"  function=0x{pc:x} caller_lr=0x{lr:x} add={add[pc,lr]} remove={remove[pc,lr]}")
    print("\nInterpretation: function-entry correlations only; validate candidates "
          "with targeted argument tracing and a full-limit test before patching.")


if __name__ == "__main__":
    main()
