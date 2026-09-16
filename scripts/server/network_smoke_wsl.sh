#!/usr/bin/env bash
set -euo pipefail

workspace="$(realpath "${1:?usage: network_smoke_wsl.sh /mnt/c/path/to/fusion32}")"
for port in 7171 7172 7173; do
  python3 - "$port" <<'PY'
import socket, sys
s=socket.socket(); s.settimeout(1)
s.connect(('127.0.0.1', int(sys.argv[1])))
s.close()
PY
  printf 'TCP_CONNECT_CLOSE port=%s PASS\n' "$port"
done
bash "$workspace/scripts/server/status_wsl.sh" "$workspace"
