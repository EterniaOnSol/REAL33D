#!/usr/bin/env bash
set -euo pipefail
root=/mnt/c/Users/dell/Desktop/fusion32
runtime=/var/lib/fusion32-server-baseline-772-0
dest=$root/build/cert-player-death-20261023/build/certification/server-before-resume
mkdir -p "$dest"
for name in querymanager game login; do
  cp -n "$runtime/$name/logs/$name.log" "$dest/$name.log" || true
done
bash "$root/scripts/server/start_wsl.sh" "$root"
