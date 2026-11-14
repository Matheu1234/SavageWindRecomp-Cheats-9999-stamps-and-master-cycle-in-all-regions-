#!/usr/bin/env bash
set -euo pipefail
if [[ ! -f Makefile || ! -f tools/gamelib_entry.c || ! -f tools/test_stamp_guest_probe.py ]]; then
  echo 'Execute este script na raiz da pasta Stamps (com overlay v3 extraido).' >&2
  exit 1
fi
# Invalidate the frozen helper and the game build fingerprint, without deleting
# third_party/, the user's own exefs, saves, or existing build dependencies.
touch tools/gen.py tools/patches.py tools/stamp_trace_sites.py tools/gamelib_entry.c Makefile
printf '%s\n' 'OK: execute make test-stamp-guest && make test-stamp-trace && make gen-helper && make dist'
printf '%s\n' 'Apos atualizar a instalacao, RECONSTRUA swgame.dll no launcher (hooks no codigo do jogo).'
