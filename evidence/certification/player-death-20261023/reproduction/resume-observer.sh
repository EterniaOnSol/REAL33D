#!/usr/bin/env bash
set -euo pipefail
cd /mnt/c/Users/dell/Desktop/fusion32/build/cert-player-death-20261023
runtime=/var/lib/fusion32-server-baseline-772-0
pid=$(cat "$runtime/game/run/game.pid")
test "$(readlink -f /proc/$pid/exe)" = "$runtime/game/bin/game"
test ! -e build/certification/live/server-observer-resumed-console.txt
sha256sum /proc/$pid/exe > build/certification/live/server-resumed-binary.sha256
exec gdb -q -batch -p "$pid" -x build/certification/observer.gdb > build/certification/live/server-observer-resumed-console.txt 2>&1
