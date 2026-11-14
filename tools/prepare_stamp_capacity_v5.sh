#!/usr/bin/env bash
# After extracting the v5 overlay into the existing Stamps source folder.
set -euo pipefail
for f in Makefile tools/patches.py tools/gamelib_entry.c \
         tools/test_stamp_capacity_v5.py 'cheats/9999 Map Stamps Limit/cheat.ini'; do
  if [[ ! -f "$f" ]]; then echo "Missing $f; extract the overlay in project root" >&2; exit 1; fi
done
if ! grep -q 'sw_stamp_capacity_stored' tools/patches.py || \
   ! grep -q 'stamp-capacity-v5.log' tools/gamelib_entry.c; then
    echo 'The v5 instrumentation source is incomplete.' >&2; exit 1
fi
# The swgen executable embeds tools/patches.py; rebuild it even if source
# archives carried future timestamps. The embedded game entry C file must
# also be distributed so the resulting DLL contains the logger.
find tools -maxdepth 1 -type f -name '*.py' -exec touch {} +
touch tools/gamelib_entry.c Makefile
if [[ -f tests/test_sw_cheat_catalog.c ]]; then touch tests/test_sw_cheat_catalog.c; fi
printf '%s\n' 'v5 instrumentation installed. Run:'
printf '%s\n' '  make test-stamp-capacity-v5'
printf '%s\n' '  make gen-helper'
printf '%s\n' '  make dist'
printf '%s\n' 'Then update the installed distribution and REBUILD swgame.dll.'
