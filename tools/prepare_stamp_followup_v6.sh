#!/usr/bin/env bash
# Run after extracting v6 overlay into the source project's root.
set -euo pipefail
for f in Makefile tools/gen.py tools/patches.py tools/gamelib_entry.c \
         tools/test_stamp_followup_v6.py tools/analyze_stamp_followup_v6.py; do
    if [[ ! -f "$f" ]]; then echo "Missing $f: extract the v6 overlay in Stamps root" >&2; exit 1; fi
done
if ! grep -q 'sw_stamp_followup_entry' tools/gen.py || \
   ! grep -q 'stamp-followup-v6.log' tools/gamelib_entry.c; then
    echo 'Incomplete v6 overlay' >&2; exit 1
fi
# Avoid stale generator, old Windows release binary and ZIP clock skew.
find tools -maxdepth 1 -name '*.py' -type f -exec touch {} +
touch tools/gamelib_entry.c Makefile
[[ -f tests/test_sw_cheat_catalog.c ]] && touch tests/test_sw_cheat_catalog.c
cat <<'MSG'
v6 follow-up instrumentation applied. From MSYS2 UCRT64:
  make test-stamp-followup-v6
  make gen-helper
  make dist
Then use tools/update_existing_install.ps1 to install the rebuilt distribution;
REBUILD compiled game code inside the Savage Wind Configuration app.
MSG
