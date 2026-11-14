#!/usr/bin/env bash
# Run after extracting the v4 overlay in /home/mr457/Stamps (MSYS2 UCRT64).
set -euo pipefail
for f in Makefile tools/gen.py tools/patches.py runtime/sw_cheats.c \
         'cheats/9999 Map Stamps Limit/cheat.ini' tools/test_stamp_cheat_v4.py; do
  if [[ ! -f "$f" ]]; then echo "Missing $f; extract overlay into project root" >&2; exit 1; fi
done
if ! grep -q 'def _stamp_capacity_9999' tools/patches.py || \
   ! grep -q 'rt_sw_stamps_9999_enabled' runtime/sw_cheats.c; then
    echo 'Missing v4 patch. Did you extract the overlay?' >&2; exit 1
fi
# Invalidate compiled host objects, but not the user's extracted game data or DLL.
rm -f build/win-release/rt/sw_cheats.o build/win-release/rt/ui/menu.o \
      build/win-release/rt/main.o
# Force the bundled code generator to be rebuilt with the new hook.
touch tools/patches.py tools/gen.py runtime/sw_cheats.c runtime/sw_cheats.h \
      runtime/ui/menu.mm Makefile
printf '%s\n' 'OK: run make test-stamp-cheat-v4 && make gen-helper && make dist'
printf '%s\n' 'After updating your installed game, Rebuild compiled game code ONCE in Settings > Prelaunch.'
