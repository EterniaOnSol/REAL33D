#!/usr/bin/env bash
set -euo pipefail

workspace="$(realpath "${1:?usage: stop_wsl.sh /mnt/c/path/to/fusion32}")"
runtime="/tmp/fusion32-server-baseline-772-${UID}"

stop_one() {
  local name="$1" signal="$2"
  local pidfile="$runtime/$name/run/$name.pid"
  local startfile="$runtime/$name/run/$name.start"
  [[ -s "$pidfile" ]] || return 0
  local pid
  pid="$(cat "$pidfile")"
  if kill -0 "$pid" 2>/dev/null; then
    if [[ ! -s "$startfile" || "$(awk '{print $22}' "/proc/$pid/stat")" != "$(cat "$startfile")" ]]; then
      echo "$name PID identity changed; refusing to signal PID $pid" >&2
      return 1
    fi
    kill -s "$signal" "$pid"
    for _ in $(seq 1 300); do
      kill -0 "$pid" 2>/dev/null || break
      sleep .1
    done
  fi
  if kill -0 "$pid" 2>/dev/null; then
    echo "$name did not stop cleanly" >&2
    return 1
  fi
  rm -f "$pidfile" "$startfile"
}

stop_one login TERM
stop_one game INT
stop_one querymanager TERM

for port in 7171 7172 7173; do
  if ss -ltnH "sport = :$port" | grep -q .; then
    echo "Port $port still listening" >&2
    exit 1
  fi
done
echo 'All services stopped cleanly.'
