#!/usr/bin/env bash
set -euo pipefail

workspace="$(realpath "${1:?usage: reset_wsl.sh /mnt/c/path/to/fusion32}")"
runtime="/tmp/fusion32-server-baseline-772-${UID}"

test -f "$workspace/AGENTS.md"
test -d "$workspace/reference/game"
case "$runtime" in
  "/tmp/fusion32-server-baseline-772-${UID}") ;;
  *) echo "Refusing unsafe reset target: $runtime" >&2; exit 2 ;;
esac

if [[ -d "$runtime" ]]; then
  bash "$workspace/scripts/server/stop_wsl.sh" "$workspace"
  rm -rf -- "$runtime"
fi

exec bash "$workspace/scripts/server/prepare_wsl.sh" "$workspace"
