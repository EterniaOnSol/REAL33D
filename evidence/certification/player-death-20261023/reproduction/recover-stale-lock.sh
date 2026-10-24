#!/usr/bin/env bash
set -euo pipefail
runtime=/var/lib/fusion32-server-baseline-772-0
lock=$runtime/game/state/save/game.pid
test "$(realpath "$lock")" = "$runtime/game/state/save/game.pid"
test "$(cat "$lock")" = 20261
test ! -d /proc/20261
test -z "$(pgrep -x game || true)"
test -z "$(ss -ltnH 'sport = :7172')"
dest=$lock.stale-player-death-20261024
test ! -e "$dest"
mv -- "$lock" "$dest"
bash /mnt/c/Users/dell/Desktop/fusion32/scripts/server/start_wsl.sh /mnt/c/Users/dell/Desktop/fusion32
