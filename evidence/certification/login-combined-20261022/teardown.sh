#!/usr/bin/env bash
set -euo pipefail
root=/mnt/c/Users/dell/Desktop/fusion32/build/cert-login-combined-20261022
out="$root/build/certification"
tracer=$(awk '/TracerPid/{print $2}' /proc/564/status)
[[ "$tracer" == 805 ]]
[[ "$(cat /proc/805/comm)" == gdb ]]
kill -TERM 805
for _ in $(seq 1 30); do
  [[ "$(awk '/TracerPid/{print $2}' /proc/564/status)" == 0 ]] && break
  sleep .1
done
[[ "$(awk '/TracerPid/{print $2}' /proc/564/status)" == 0 ]]
grep TracerPid /proc/564/status > "$out/teardown.txt"
bash /mnt/c/Users/dell/Desktop/fusion32/scripts/server/stop_wsl.sh /mnt/c/Users/dell/Desktop/fusion32 >> "$out/teardown.txt"
grep 'Test Player B' /var/lib/fusion32-server-baseline-772-0/game/logs/game.log > "$out/server-player-log.txt"