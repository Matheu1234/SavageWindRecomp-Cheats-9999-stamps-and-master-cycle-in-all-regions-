#!/usr/bin/env bash
# Run once after overlay extraction, from the Savage Wind project's root.
# Preserve the existing build cache, FFmpeg dependencies and user-supplied fonts.
set -euo pipefail
if [[ ! -f Makefile || ! -f runtime/stamp_flow.c || ! -f runtime/fnprof.c ]]; then
    echo 'ERROR: execute na raiz do projeto Stamps após aplicar o overlay.' >&2
    exit 1
fi
if ! grep -q 'rt_sw_stamp_flow_hit' runtime/fnprof.c ||
   ! grep -q 'rt_sw_stamp_flow_init' runtime/main.c; then
    echo 'ERROR: a atualização está incompleta; extraia o overlay novamente.' >&2
    exit 1
fi
# Old build/ objects may have misleading newer timestamps (clock skew).
# Only the relevant host runtime objects are invalidated; swgame.dll stays.
rm -f build/win-release/rt/fnprof.o \
      build/win-release/rt/main.o \
      build/win-release/rt/stamp_flow.o
touch Makefile runtime/fnprof.c runtime/main.c runtime/stamp_flow.c runtime/stamp_flow.h
printf '%s\n' 'OK: host runtime prepared. Now run: make test-stamp-flow && make dist'
