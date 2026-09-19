#!/usr/bin/env bash
set -euo pipefail

workspace="$(realpath "${1:?usage: start_wsl.sh /mnt/c/path/to/fusion32}")"
runtime="/var/lib/fusion32-server-baseline-772-${UID}"
umask 077

start_one() {
  local name="$1" cwd="$2" executable="$3"
  local pidfile="$cwd/run/$name.pid"
  if [[ -s "$pidfile" ]]; then
    if kill -0 "$(cat "$pidfile")" 2>/dev/null; then
      echo "$name is already running (PID $(cat "$pidfile"))" >&2
      exit 2
    fi
    rm -f "$pidfile"
  fi
  (
    cd "$cwd"
    nohup "$executable" >"logs/$name.log" 2>&1 &
    pid="$!"
    printf '%s\n' "$pid" >"$pidfile"
    for _ in $(seq 1 100); do
      [[ -r "/proc/$pid/stat" ]] && break
      sleep .01
    done
    [[ -r "/proc/$pid/stat" ]]
    awk '{print $22}' "/proc/$pid/stat" >"$cwd/run/$name.start"
  )
}

wait_log_marker() {
  local logfile="$1" marker="$2" name="$3" pidfile="$4"
  for _ in $(seq 1 2400); do
    if grep -q "$marker" "$logfile" 2>/dev/null; then return 0; fi
    if [[ -s "$pidfile" ]] && ! kill -0 "$(cat "$pidfile")" 2>/dev/null; then
      echo "$name exited before readiness marker: $marker" >&2
      return 1
    fi
    sleep .25
  done
  echo "$name did not reach readiness marker within 600 seconds: $marker" >&2
  return 1
}

cleanup_on_error() {
  bash "$workspace/scripts/server/stop_wsl.sh" "$workspace" || true
}
trap cleanup_on_error ERR

wait_port() {
  local port="$1" name="$2"
  for _ in $(seq 1 100); do
    if python3 - "$port" <<'PY' >/dev/null 2>&1
import socket, sys
s=socket.socket(); s.settimeout(.1)
try: s.connect(('127.0.0.1', int(sys.argv[1])))
except OSError: raise SystemExit(1)
finally: s.close()
PY
    then return 0; fi
    sleep .1
  done
  echo "$name did not bind port $port" >&2
  return 1
}

start_one querymanager "$runtime/querymanager" "$runtime/querymanager/bin/querymanager"
wait_port 7173 querymanager
start_one game "$runtime/game" "$runtime/game/bin/game"
wait_port 7172 game
wait_log_marker "$runtime/game/logs/game.log" 'Sektoren geladen.' game "$runtime/game/run/game.pid"
start_one login "$runtime/login" "$runtime/login/bin/login"
wait_port 7171 login

bash "$workspace/scripts/server/status_wsl.sh" "$workspace"
trap - ERR
