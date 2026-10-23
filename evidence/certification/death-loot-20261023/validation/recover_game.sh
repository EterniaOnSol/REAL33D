#!/usr/bin/env bash
set -euo pipefail
root=/mnt/c/Users/dell/Desktop/fusion32/build/cert-death-loot-20261023
runtime=/var/lib/fusion32-server-baseline-772-0
out="$root/build/certification"
oldpid="$(cat "$runtime/game/run/game.pid")"
if kill -0 "$oldpid" 2>/dev/null; then echo 'Recorded game process still alive; refusing restart'; exit 1; fi
if ss -ltnH 'sport = :7172' | grep -q .; then echo 'Game port occupied; refusing restart'; exit 1; fi
cp -n "$runtime/game/logs/game.log" "$out/game-before-recovery.log"
cd "$runtime/game"
nohup ./bin/game >logs/game.log 2>&1 </dev/null &
newpid=$!
printf '%s\n' "$newpid" >run/game.pid
awk '{print $22}' "/proc/$newpid/stat" >run/game.start
printf 'Started existing verified game binary as PID %s\n' "$newpid"
