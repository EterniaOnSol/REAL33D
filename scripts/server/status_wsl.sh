#!/usr/bin/env bash
set -euo pipefail

workspace="$(realpath "${1:?usage: status_wsl.sh /mnt/c/path/to/fusion32}")"
runtime="/var/lib/fusion32-server-baseline-772-${UID}"
failed=0

check_process() {
  local name="$1" port="$2"
  pidfile="$runtime/$name/run/$name.pid"
  startfile="$runtime/$name/run/$name.start"
  expected_exe="$(realpath "$runtime/$name/bin/$name")"
  expected_cwd="$(realpath "$runtime/$name")"
  if [[ ! -s "$pidfile" || ! -s "$startfile" ]]; then
    printf '%s DEAD\n' "$name"
    failed=1
    return
  fi
  local pid recorded_start actual_start actual_exe actual_cwd
  pid="$(cat "$pidfile")"
  if ! kill -0 "$pid" 2>/dev/null || [[ ! -r "/proc/$pid/stat" ]]; then
    printf '%s DEAD\n' "$name"
    failed=1
    return
  fi
  recorded_start="$(cat "$startfile")"
  actual_start="$(awk '{print $22}' "/proc/$pid/stat")"
  actual_exe="$(readlink -f "/proc/$pid/exe")"
  actual_cwd="$(readlink -f "/proc/$pid/cwd")"
  if [[ "$actual_start" != "$recorded_start" || "$actual_exe" != "$expected_exe" || "$actual_cwd" != "$expected_cwd" ]]; then
    printf '%s IDENTITY_MISMATCH pid=%s\n' "$name" "$pid"
    failed=1
    return
  fi
  if ! ss -ltnpH "sport = :$port" | grep -Fq "pid=$pid,"; then
    printf '%s PORT_OWNERSHIP_MISMATCH port=%s pid=%s\n' "$name" "$port" "$pid"
    failed=1
    return
  fi
  printf '%s ALIVE pid=%s exe=VERIFIED cwd=VERIFIED start=VERIFIED port=%s\n' "$name" "$pid" "$port"
}

check_process querymanager 7173
check_process game 7172
check_process login 7171

grep -q 'Running...' "$runtime/querymanager/logs/querymanager.log" || { echo 'QM readiness marker missing'; failed=1; }
grep -q 'Sektoren geladen.' "$runtime/game/logs/game.log" || { echo 'Game map-load marker missing'; failed=1; }
grep -q 'Objekte geladen.' "$runtime/game/logs/game.log" || { echo 'Game object-load marker missing'; failed=1; }
grep -q 'Running...' "$runtime/login/logs/login.log" || { echo 'Login readiness marker missing'; failed=1; }
grep -q 'AUTHORIZED to game server' "$runtime/querymanager/logs/querymanager.log" || { echo 'QM game authorization marker missing'; failed=1; }
grep -q 'AUTHORIZED to login server' "$runtime/querymanager/logs/querymanager.log" || { echo 'QM login authorization marker missing'; failed=1; }

exit "$failed"
