#!/usr/bin/env bash
set -euo pipefail
cd /mnt/c/Users/dell/Desktop/fusion32/build/cert-player-death-20261023
mkdir -p build/certification/live
test ! -e build/certification/live/server-trace.jsonl
test "$(readlink -f /proc/20261/exe)" = /var/lib/fusion32-server-baseline-772-0/game/bin/game
sha256sum /proc/20261/exe > build/certification/live/server-binary.sha256
exec gdb -q -batch -p 20261 -x build/certification/observer.gdb > build/certification/live/server-observer-console.txt 2>&1
