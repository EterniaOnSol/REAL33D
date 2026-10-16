#!/usr/bin/env bash
# Self-test for the two Windows checks in tests/regression_harness.sh.
#
# Why this file exists. Both checks used to be written as
#
#     ( cd "$REPO" && cmd.exe /c "<command>" ) >"$log" 2>&1
#
# and from Git Bash that never ran anything. MSYS rewrites an argument that
# looks like an absolute Unix path, so `/c` arrived as `C:/`; cmd never saw
# the switch, started interactively, read EOF from the redirected stdin,
# printed its banner and exited 0. The checks keyed on exit status alone, so
# both recorded PASS. A gate that cannot fail is worse than no gate, and two
# published reports carried the overstated rows before it was noticed.
#
# So this test asserts the two properties that failure violated:
#
#   1. the command really executes (it leaves a sentinel on disk), and
#   2. exit 0 without the build's own success line is FAIL, never PASS.
#
# It drives the real harness against a throwaway fixture repository whose
# batch files this test writes, so it exercises the real cmd.exe on the real
# host rather than a mock of it.
#
# Usage: bash tests/regression_harness_selftest.sh
# Exit: 0 all assertions held, 1 an assertion failed, 2 could not run.
set -uo pipefail

HARNESS="$(cd "$(dirname "$0")" && pwd)/regression_harness.sh"
[ -f "$HARNESS" ] || { echo "cannot find regression_harness.sh beside this test" >&2; exit 2; }

command -v cmd.exe >/dev/null 2>&1 || {
  echo "SKIP: no cmd.exe on this host; the Windows checks cannot be exercised here"
  exit 0
}

pass=0; fail=0
ok()  { printf 'ok   : %s\n' "$1"; pass=$((pass + 1)); }
bad() { printf 'FAIL : %s\n' "$1" >&2; fail=$((fail + 1)); }

ROOT="$(mktemp -d)"
trap 'rm -rf "$ROOT"' EXIT

# ------------------------------------------------------------------ fixture ---
# A minimal repository the harness will accept: a git worktree on a branch
# that is not main, a .uproject, and the two batch files the checks launch.
FIX="$ROOT/repo"
mkdir -p "$FIX/tests" "$FIX/unreal/REAL33D"
: >"$FIX/unreal/REAL33D/REAL33D.uproject"
( cd "$FIX" \
  && git init -q \
  && git checkout -q -b selftest-fixture \
  && git -c user.email=t@t -c user.name=t commit -q --allow-empty -m fixture ) || {
  echo "could not create the fixture repository" >&2; exit 2; }

ENGINE_DIR="$ROOT/engine"
mkdir -p "$ENGINE_DIR/Engine/Build/BatchFiles"
BUILD_BAT="$ENGINE_DIR/Engine/Build/BatchFiles/Build.bat"
MSVC_CMD="$FIX/tests/build_clientcore_windows.cmd"

win_path() {
  if command -v cygpath >/dev/null 2>&1; then cygpath -w "$1"; else printf '%s' "$1"; fi
}
ENGINE_WIN="$(win_path "$ENGINE_DIR")"

# Each case rewrites the batch files, then reads the status the harness
# recorded for one check out of its own output.
run_harness() { # extra args...
  local out="$ROOT/out-$RANDOM"
  bash "$HARNESS" --repo-root "$FIX" --out "$out" --no-wsl \
    --engine "$ENGINE_WIN" "$@" 2>/dev/null
}
status_of() { # check-name < harness output
  awk -v want="$1" '$1 == want { print $2; exit }'
}

# ----------------------------------------------------- 1. the command runs ---
# The exact regression: the batch file leaves a sentinel. If the sentinel is
# missing, cmd.exe was launched in a way that ran nothing, which is the bug.
SENTINEL="$ROOT/sentinel.txt"
rm -f "$SENTINEL"
printf '@echo off\r\necho ran > "%s"\r\necho Result: Succeeded\r\n' \
  "$(win_path "$SENTINEL")" >"$BUILD_BAT"
out="$(run_harness --unreal)"
if [ -f "$SENTINEL" ]; then
  ok "the Unreal check actually executes its command (sentinel written)"
else
  bad "the Unreal check did not execute anything; cmd.exe invocation is broken again"
fi
if [ "$(printf '%s' "$out" | status_of unreal-module-build)" = "PASS" ]; then
  ok "a real build that prints its success line is PASS"
else
  bad "a real successful build was not recorded as PASS"
fi

# ------------------------------- 2. exit 0 without the marker is not a PASS ---
# This reproduces the old failure exactly: a command that exits 0 and prints
# only a banner, the way an interactive cmd.exe did.
printf '@echo off\r\necho Microsoft Windows [Version 10.0]\r\nexit /b 0\r\n' >"$BUILD_BAT"
got="$(run_harness --unreal | status_of unreal-module-build)"
if [ "$got" = "FAIL" ]; then
  ok "no-op cmd.exe exit 0 is FAIL for unreal-module-build, not PASS"
else
  bad "no-op cmd.exe exit 0 produced '$got' for unreal-module-build; expected FAIL"
fi

printf '@echo off\r\necho Microsoft Windows [Version 10.0]\r\nexit /b 0\r\n' >"$MSVC_CMD"
got="$(run_harness | status_of build-sanity-msvc)"
case "$got" in
  FAIL) ok "no-op cmd.exe exit 0 is FAIL for build-sanity-msvc, not PASS" ;;
  SKIP) ok "build-sanity-msvc SKIPped (no cl.exe on PATH); run from a developer environment to assert FAIL" ;;
  *)    bad "no-op cmd.exe exit 0 produced '$got' for build-sanity-msvc; expected FAIL" ;;
esac

printf '@echo off\r\necho WINDOWS CLIENTCORE: PASS\r\n' >"$MSVC_CMD"
got="$(run_harness | status_of build-sanity-msvc)"
case "$got" in
  PASS) ok "build-sanity-msvc is PASS when its success line is present" ;;
  SKIP) ok "build-sanity-msvc SKIPped (no cl.exe on PATH)" ;;
  *)    bad "build-sanity-msvc produced '$got' with its success line present; expected PASS" ;;
esac

# ------------------------------------- 3. a failing command is still a FAIL ---
printf '@echo off\r\necho Result: Succeeded\r\nexit /b 3\r\n' >"$BUILD_BAT"
got="$(run_harness --unreal | status_of unreal-module-build)"
if [ "$got" = "FAIL" ]; then
  ok "a non-zero exit is FAIL even when the success line was printed"
else
  bad "a command exiting 3 produced '$got'; expected FAIL"
fi

# ------------------------------------- 4. an absent toolchain is still SKIP ---
# SKIP must survive the fix: a missing engine is not a broken build.
rm -f "$BUILD_BAT"
got="$(run_harness --unreal | status_of unreal-module-build)"
if [ "$got" = "SKIP" ]; then
  ok "a missing engine is SKIP, not FAIL and not PASS"
else
  bad "a missing engine produced '$got'; expected SKIP"
fi

printf '\n%d passed, %d failed\n' "$pass" "$fail"
[ "$fail" -eq 0 ] || exit 1
exit 0
