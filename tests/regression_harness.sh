#!/usr/bin/env bash
# REAL33D 3D regression harness.
#
# One reproducible command for every 3D milestone. It records where the tree
# actually was, runs the checks that need neither a live Fusion32 nor a human
# at the GUI, and writes a Markdown report with one explicit status per check.
#
# Three statuses, never two: PASS, FAIL and SKIP. A check whose toolchain is
# absent is SKIP with the reason printed, never PASS. The whole point of this
# script is that its report can be trusted, so it must never claim a result it
# did not obtain.
#
# It is read-only with respect to the repository. Everything it writes goes
# under --out, which defaults inside the gitignored build/ tree, so a run
# leaves the worktree clean. It starts no server, touches no gameplay, builds
# no Unreal runtime unless asked, and never prints environment values.
#
# Exit: 0 no failures, 1 at least one FAIL (or a SKIP under --strict),
#       2 bad usage.
set -uo pipefail

usage() {
  cat <<'EOF'
Usage: tests/regression_harness.sh [options]

  --repo-root DIR    repository root (default: the parent of this script)
  --out DIR          report directory (default: REPO/build/regression-harness/UTC)
  --wsl-distro NAME  WSL distro to borrow a toolchain from (default: Ubuntu-26.04)
  --no-wsl           never fall back to WSL
  --objects-srv PATH live objects.srv; enables the runtime MultiUse assertions
  --logs PATH        extra file or directory to scan for protocol anomalies
                     (repeatable)
  --logs-all         scan every log under evidence/ instead of only the ones
                     this branch added or changed
  --unreal           also build the Unreal module with UnrealBuildTool
  --engine DIR       Unreal engine root (default: C:\Program Files\Epic Games\UE_5.8)
  --allow-main       do not fail the branch-isolation check when on main
  --strict           exit non-zero when any check is SKIP
  -h, --help         this text
EOF
}

script_dir="$(cd "$(dirname "$0")" && pwd)"
REPO="$(cd "$script_dir/.." && pwd)"
OUT=""
WSL_DISTRO="Ubuntu-26.04"
USE_WSL=1
OBJECTS_SRV=""
EXTRA_LOGS=()
LOGS_ALL=0
WANT_UNREAL=0
ENGINE="C:\\Program Files\\Epic Games\\UE_5.8"
ALLOW_MAIN=0
STRICT=0

while [ $# -gt 0 ]; do
  case "$1" in
    --repo-root)   REPO="$(cd "$2" && pwd)"; shift 2 ;;
    --out)         OUT="$2"; shift 2 ;;
    --wsl-distro)  WSL_DISTRO="$2"; shift 2 ;;
    --no-wsl)      USE_WSL=0; shift ;;
    --objects-srv) OBJECTS_SRV="$2"; shift 2 ;;
    --logs)        EXTRA_LOGS+=("$2"); shift 2 ;;
    --logs-all)    LOGS_ALL=1; shift ;;
    --unreal)      WANT_UNREAL=1; shift ;;
    --engine)      ENGINE="$2"; shift 2 ;;
    --allow-main)  ALLOW_MAIN=1; shift ;;
    --strict)      STRICT=1; shift ;;
    -h|--help)     usage; exit 0 ;;
    *) printf 'unknown option: %s\n' "$1" >&2; usage >&2; exit 2 ;;
  esac
done

cd "$REPO" || { echo "cannot enter $REPO" >&2; exit 2; }
STAMP="$(date -u +%Y%m%dT%H%M%SZ)"
[ -n "$OUT" ] || OUT="$REPO/build/regression-harness/$STAMP"
mkdir -p "$OUT/logs" || { echo "cannot create $OUT" >&2; exit 2; }
REPORT="$OUT/REGRESSION-HARNESS-$STAMP.md"

# ---------------------------------------------------------------- results ---
CHECK_NAMES=(); CHECK_STATUS=(); CHECK_SUMMARY=(); CHECK_LOG=()

record() { # name status summary [log-stem]
  CHECK_NAMES+=("$1"); CHECK_STATUS+=("$2"); CHECK_SUMMARY+=("$3"); CHECK_LOG+=("${4:-}")
  printf '%-22s %-5s %s\n' "$1" "$2" "$3"
}

logfile() { printf '%s/logs/%s.txt' "$OUT" "$1"; }

# Git Bash reports /c/..., WSL needs /mnt/c/.... wslpath is authoritative when
# this script is itself running inside WSL.
to_wsl_path() {
  local p="$1" drive rest
  if command -v wslpath >/dev/null 2>&1; then wslpath -a "$p"; return; fi
  case "$p" in
    /[a-zA-Z]/*)
      drive="$(printf '%s' "${p:1:1}" | tr 'A-Z' 'a-z')"; rest="${p:2}"
      printf '/mnt/%s%s' "$drive" "$rest" ;;
    *) printf '%s' "$p" ;;
  esac
}

have_wsl() {
  [ "$USE_WSL" -eq 1 ] && command -v wsl.exe >/dev/null 2>&1 \
    && wsl.exe -d "$WSL_DISTRO" -- true >/dev/null 2>&1
}

to_win_path() {
  local p="$1"
  if command -v cygpath >/dev/null 2>&1; then cygpath -w "$p"; return; fi
  if command -v wslpath >/dev/null 2>&1; then wslpath -w "$p"; return; fi
  printf '%s' "$p"
}

# Run a Windows command line and return its real exit status.
#
# Three hazards, each of which produced a silent false PASS before this helper
# existed:
#
#   1. MSYS rewrites any argument that looks like an absolute Unix path, so a
#      bare `cmd.exe /c ...` from Git Bash arrives as `cmd.exe C:/ ...`. cmd
#      never sees the switch, starts interactively, reads EOF from the
#      redirected stdin, prints its banner and exits 0 having run nothing.
#      That is exactly the shape of a check that cannot fail, which is the one
#      outcome this script exists to prevent.
#   2. cmd's own quote stripping for `/c "<line>"` mangles a command whose
#      program path is itself quoted, which every engine path here is.
#   3. An interactive prompt would otherwise block until the CI timeout.
#
# The line is therefore written to a temporary .cmd and executed by path, with
# argument conversion disabled and stdin closed. Quoting inside the line is
# then plain Windows batch and nothing rewrites it.
#
# The switch stays `/c`, not `//c`. Both run under Git Bash, but for opposite
# reasons: `//c` works only *because* MSYS collapses it back to `/c`, so it
# breaks the moment conversion is turned off and is wrong on a non-MSYS host.
# `/c` plus MSYS_NO_PATHCONV is correct everywhere, because the variables are
# simply ignored where they mean nothing. Disabling conversion and writing
# `//c` cancel each other out and silently restore the original bug, which is
# what tests/regression_harness_selftest.sh case 1 is there to catch.
#
# Returns 127 when there is no cmd.exe at all. Callers must map that to SKIP,
# never to FAIL: a Linux CI box is not a broken build.
run_windows_command() { # workdir logfile line
  local workdir="$1" log="$2" line="$3" script rc
  command -v cmd.exe >/dev/null 2>&1 || return 127
  script="$OUT/.run-$$.cmd"
  printf '@echo off\r\ncd /d "%s" || exit /b 1\r\n%s\r\nexit /b %%ERRORLEVEL%%\r\n' \
    "$(to_win_path "$workdir")" "$line" >"$script"
  MSYS_NO_PATHCONV=1 MSYS2_ARG_CONV_EXCL='*' \
    cmd.exe /c "$(to_win_path "$script")" </dev/null >"$log" 2>&1
  rc=$?
  rm -f "$script"
  return "$rc"
}

# A zero exit is necessary but not sufficient. Both Windows checks below exist
# to prove that a specific build really ran, so each also requires the success
# line that build prints for itself. Exit status alone is what a no-op cmd.exe
# was able to satisfy.
MSVC_SUCCESS_MARKER='WINDOWS CLIENTCORE: PASS'
UNREAL_SUCCESS_MARKER='Result: Succeeded'

log_has() { grep -qF -- "$2" "$1" 2>/dev/null; }

# Resolved once, and printed in the report, so a reader of a PASS knows which
# compiler produced it.
TOOLCHAIN="none"; TOOLCHAIN_NOTE="no C++ compiler and no usable WSL distro found"
native_cxx=""
for c in "${CXX:-}" g++ clang++ c++; do
  [ -n "$c" ] && command -v "$c" >/dev/null 2>&1 && { native_cxx="$c"; break; }
done
if [ -n "$native_cxx" ] && command -v cmake >/dev/null 2>&1; then
  TOOLCHAIN="native"
  TOOLCHAIN_NOTE="native, $("$native_cxx" --version 2>/dev/null | head -1)"
elif have_wsl && wsl.exe -d "$WSL_DISTRO" -- bash -c 'command -v g++ >/dev/null && command -v cmake >/dev/null' >/dev/null 2>&1; then
  TOOLCHAIN="wsl"
  TOOLCHAIN_NOTE="WSL $WSL_DISTRO, $(wsl.exe -d "$WSL_DISTRO" -- g++ --version 2>/dev/null | head -1 | tr -d '\r')"
fi

# ------------------------------------------------------ check: provenance ---
git_ok=0
git rev-parse --is-inside-work-tree >/dev/null 2>&1 && git_ok=1

BRANCH="(unknown)"; HEAD_SHA="(unknown)"; HEAD_SUBJECT=""; ORIGIN_MAIN="(unknown)"
AHEAD="?"; BEHIND="?"; DIRTY_COUNT="?"; MERGE_BASE=""
if [ "$git_ok" -eq 1 ]; then
  BRANCH="$(git rev-parse --abbrev-ref HEAD 2>/dev/null)"
  HEAD_SHA="$(git rev-parse HEAD 2>/dev/null)"
  HEAD_SUBJECT="$(git log -1 --format=%s 2>/dev/null)"
  if git rev-parse --verify -q origin/main >/dev/null; then
    ORIGIN_MAIN="$(git rev-parse origin/main)"
    MERGE_BASE="$(git merge-base HEAD origin/main 2>/dev/null)"
    read -r BEHIND AHEAD <<<"$(git rev-list --left-right --count origin/main...HEAD 2>/dev/null)"
  else
    ORIGIN_MAIN="(no origin/main)"
  fi
  DIRTY_COUNT="$(git status --porcelain 2>/dev/null | wc -l | tr -d ' ')"
  git worktree list >"$(logfile provenance-worktrees)" 2>&1
  git status --porcelain >"$(logfile provenance-status)" 2>&1
  record provenance PASS \
    "branch $BRANCH at ${HEAD_SHA:0:8}, $AHEAD ahead / $BEHIND behind origin/main, $DIRTY_COUNT uncommitted path(s)" \
    provenance-status
else
  record provenance FAIL "not inside a git worktree; provenance cannot be recorded"
fi

# ------------------------------------------------ check: branch isolation ---
# Milestone work never runs from main in this repo. Catching it here is
# cheaper than catching it in review.
if [ "$git_ok" -eq 0 ]; then
  record branch-isolation SKIP "no git metadata to check"
elif [ "$ALLOW_MAIN" -eq 1 ]; then
  record branch-isolation SKIP "--allow-main was passed"
elif [ "$BRANCH" = "main" ] || [ "$BRANCH" = "master" ]; then
  record branch-isolation FAIL "HEAD is on $BRANCH; milestone work belongs on its own branch"
else
  record branch-isolation PASS "on $BRANCH, not on main"
fi

# --------------------------------------------- check: ClientCore CTest suites ---
# The canonical Protocol772Core suites. Nothing here needs the network,
# Fusion32, the classic client or Unreal.
cc_log="$(logfile clientcore-tests)"
rc=127
case "$TOOLCHAIN" in
  native)
    bdir="$OUT/clientcore-build"
    { cmake -S "$REPO/clientcore" -B "$bdir" -DCMAKE_BUILD_TYPE=Debug \
        && cmake --build "$bdir" --parallel \
        && ctest --test-dir "$bdir" --output-on-failure; } >"$cc_log" 2>&1
    rc=$?
    ;;
  wsl)
    wrepo="$(to_wsl_path "$REPO")"
    wbuild="/tmp/real33d-harness-clientcore-$$"
    wsl.exe -d "$WSL_DISTRO" -- bash -c "
      cmake -S '$wrepo/clientcore' -B '$wbuild' -DCMAKE_BUILD_TYPE=Debug \
        && cmake --build '$wbuild' --parallel \
        && ctest --test-dir '$wbuild' --output-on-failure
      rc=\$?; rm -rf '$wbuild'; exit \$rc" >"$cc_log" 2>&1
    rc=$?
    ;;
esac
if [ "$TOOLCHAIN" = "none" ]; then
  record clientcore-tests SKIP "$TOOLCHAIN_NOTE" clientcore-tests
elif [ "$rc" -eq 0 ]; then
  passed="$(grep -cE 'Test +#[0-9]+:.*Passed' "$cc_log" 2>/dev/null)"
  record clientcore-tests PASS "$passed CTest suite(s) passed via $TOOLCHAIN" clientcore-tests
else
  record clientcore-tests FAIL "configure/build/ctest returned $rc via $TOOLCHAIN" clientcore-tests
fi

# ----------------------------------- check: REAL33D standalone unit tests ---
# tests/real33d_*_tests.cpp are header-only and link nothing from Unreal, so
# they are the 3D-side logic a plain compiler can still prove.
r33_log="$(logfile real33d-native-tests)"
: >"$r33_log"
r33_pass=0; r33_fail=0

run_one_native() { # stem
  local stem="$1" exe="$OUT/$1" args=()
  [ "$stem" = "real33d_item_use_policy_tests" ] && [ -n "$OBJECTS_SRV" ] && args=("$OBJECTS_SRV")
  "$native_cxx" -std=c++17 -Wall -Wextra -Werror \
    -I"$REPO/unreal/REAL33D/Source/REAL33D/Public" \
    -o "$exe" "$REPO/tests/$stem.cpp" >>"$r33_log" 2>&1 || return 1
  "$exe" ${args[@]+"${args[@]}"} >>"$r33_log" 2>&1
}

run_one_wsl() { # stem
  local stem="$1" wrepo warg=""
  wrepo="$(to_wsl_path "$REPO")"
  [ "$stem" = "real33d_item_use_policy_tests" ] && [ -n "$OBJECTS_SRV" ] \
    && warg="'$(to_wsl_path "$OBJECTS_SRV")'"
  wsl.exe -d "$WSL_DISTRO" -- bash -c "
    out=\$(mktemp -d)
    g++ -std=c++17 -Wall -Wextra -Werror \
      -I'$wrepo/unreal/REAL33D/Source/REAL33D/Public' \
      -o \"\$out/$stem\" '$wrepo/tests/$stem.cpp' || { rm -rf \"\$out\"; exit 1; }
    \"\$out/$stem\" $warg; rc=\$?; rm -rf \"\$out\"; exit \$rc" >>"$r33_log" 2>&1
}

if [ "$TOOLCHAIN" = "none" ]; then
  record real33d-native-tests SKIP "$TOOLCHAIN_NOTE" real33d-native-tests
else
  for stem in real33d_mouse_input_tests real33d_minimap_view_tests real33d_item_use_policy_tests; do
    if [ ! -f "$REPO/tests/$stem.cpp" ]; then
      printf '=== %s === SOURCE MISSING\n' "$stem" >>"$r33_log"
      r33_fail=$((r33_fail + 1)); continue
    fi
    printf '=== %s ===\n' "$stem" >>"$r33_log"
    if [ "$TOOLCHAIN" = "native" ]; then run_one_native "$stem"; else run_one_wsl "$stem"; fi
    if [ $? -eq 0 ]; then r33_pass=$((r33_pass + 1)); else r33_fail=$((r33_fail + 1)); fi
  done
  note="$r33_pass passed, $r33_fail failed via $TOOLCHAIN"
  [ -n "$OBJECTS_SRV" ] || note="$note; no --objects-srv, so the runtime MultiUse assertions were not exercised"
  if [ "$r33_fail" -eq 0 ]; then
    record real33d-native-tests PASS "$note" real33d-native-tests
  else
    record real33d-native-tests FAIL "$note" real33d-native-tests
  fi
fi

# ----------------------------------------------------- check: secret check ---
sc_log="$(logfile secret-check)"
if [ ! -f "$REPO/tests/secret_check.sh" ]; then
  record secret-check SKIP "tests/secret_check.sh not present"
else
  bash "$REPO/tests/secret_check.sh" "$REPO" >"$sc_log" 2>&1
  rc=$?
  if [ "$rc" -eq 0 ]; then
    record secret-check PASS "$(grep -c '^ok   :' "$sc_log") sub-check(s) ok" secret-check
  else
    record secret-check FAIL "secret_check.sh exited $rc" secret-check
  fi
fi

# -------------------------------------- check: MSVC build sanity (Windows) ---
# This is the archive the Unreal module actually links, compiled the way UE
# compiles it. The cheapest build sanity that still proves the real
# combination, and it needs no engine build.
msvc_log="$(logfile build-sanity-msvc)"
if [ ! -f "$REPO/tests/build_clientcore_windows.cmd" ]; then
  record build-sanity-msvc SKIP "tests/build_clientcore_windows.cmd not present"
elif ! command -v cl.exe >/dev/null 2>&1; then
  record build-sanity-msvc SKIP "cl.exe not on PATH; run from an x64 Native Tools prompt or call vcvars64.bat first"
else
  run_windows_command "$REPO" "$msvc_log" 'tests\build_clientcore_windows.cmd "'"$ENGINE"'"'
  rc=$?
  if [ "$rc" -eq 127 ]; then
    record build-sanity-msvc SKIP "no cmd.exe on this host; the Windows build cannot be launched" build-sanity-msvc
  elif [ "$rc" -ne 0 ]; then
    record build-sanity-msvc FAIL "build_clientcore_windows.cmd exited $rc" build-sanity-msvc
  elif ! log_has "$msvc_log" "$MSVC_SUCCESS_MARKER"; then
    record build-sanity-msvc FAIL \
      "exit 0 but the log does not contain '$MSVC_SUCCESS_MARKER'; the build did not run" build-sanity-msvc
  else
    record build-sanity-msvc PASS "C++17 suites and the C++20 archive Unreal links both built" build-sanity-msvc
  fi
fi

# ---------------------------------------- check: Unreal module build (opt) ---
# Off by default on purpose: a full editor build is long and needs MSVC plus a
# populated engine, which is not a safe default for CI. Ask for it explicitly.
ub_log="$(logfile unreal-module-build)"
engine_probe="$(printf '%s' "$ENGINE" | tr '\\' '/')/Engine/Build/BatchFiles/Build.bat"
uproject="$(ls "$REPO"/unreal/REAL33D/*.uproject 2>/dev/null | head -1)"
if [ "$WANT_UNREAL" -eq 0 ] && [ -f "$engine_probe" ]; then
  record unreal-module-build SKIP "engine present but --unreal not passed; a full module build is not a CI default"
elif [ "$WANT_UNREAL" -eq 0 ]; then
  record unreal-module-build SKIP "--unreal not passed and no engine at $ENGINE"
elif [ ! -f "$engine_probe" ]; then
  record unreal-module-build SKIP "no Build.bat under $ENGINE; pass --engine DIR"
elif ! command -v cl.exe >/dev/null 2>&1; then
  record unreal-module-build SKIP "cl.exe not on PATH; UnrealBuildTool cannot compile the module"
elif [ -z "$uproject" ]; then
  record unreal-module-build SKIP "no .uproject under unreal/REAL33D"
else
  win_uproject="$(to_win_path "$uproject")"
  run_windows_command "$REPO" "$ub_log" \
    '"'"$ENGINE"'\Engine\Build\BatchFiles\Build.bat" REAL33DEditor Win64 Development -Project="'"$win_uproject"'" -WaitMutex'
  rc=$?
  if [ "$rc" -eq 127 ]; then
    record unreal-module-build SKIP "no cmd.exe on this host; UnrealBuildTool cannot be launched" unreal-module-build
  elif [ "$rc" -ne 0 ]; then
    record unreal-module-build FAIL "UnrealBuildTool exited $rc" unreal-module-build
  elif ! log_has "$ub_log" "$UNREAL_SUCCESS_MARKER"; then
    record unreal-module-build FAIL \
      "exit 0 but the log does not contain '$UNREAL_SUCCESS_MARKER'; the build did not run" unreal-module-build
  else
    record unreal-module-build PASS "REAL33DEditor Win64 Development built" unreal-module-build
  fi
fi

# ------------------------- check: protocol anomalies in logs already on disk ---
# Three counters the bridge writes into every evidence snapshot, plus the
# diagnostics it emits in words. A nonzero counter means the client walked off
# the end of a server command: the failure this project cares about most, and
# the one easiest to leave unread at the bottom of a JSON file.
#
# Default scope is what this branch added or changed against origin/main.
# Scanning all of evidence/ instead would re-report historical, already
# adjudicated numbers on every run until nobody reads the check.
pl_log="$(logfile protocol-log-check)"
: >"$pl_log"
scan_targets=()
if [ "$LOGS_ALL" -eq 1 ]; then
  [ -d "$REPO/evidence" ] && scan_targets+=("$REPO/evidence")
  printf 'scope: every log under evidence/ (--logs-all)\n\n' >>"$pl_log"
elif [ "$git_ok" -eq 1 ] && [ -n "$MERGE_BASE" ]; then
  printf 'scope: evidence logs added or changed against origin/main (%s)\n\n' "${MERGE_BASE:0:8}" >>"$pl_log"
  # Machine-written logs only. A milestone write-up is prose that quotes the
  # very diagnostics this check greps for, so sweeping every changed file
  # under evidence/ would flag the document that reports the anomaly as if it
  # were the anomaly. Explicit --logs paths below stay unfiltered: naming a
  # file is the caller saying they meant that file.
  while IFS= read -r f; do
    case "$f" in
      *.json|*.jsonl|*.txt|*.log) ;;
      *) continue ;;
    esac
    [ -n "$f" ] && [ -f "$REPO/$f" ] && scan_targets+=("$REPO/$f")
  done < <(
    { git diff --name-only --diff-filter=ACMR "$MERGE_BASE"..HEAD -- evidence 2>/dev/null
      git diff --name-only --diff-filter=ACMR -- evidence 2>/dev/null
      git ls-files --others --exclude-standard -- evidence 2>/dev/null
    } | sort -u )
else
  printf 'scope: --logs paths only (no git base available)\n\n' >>"$pl_log"
fi
for extra in ${EXTRA_LOGS[@]+"${EXTRA_LOGS[@]}"}; do
  [ -e "$extra" ] && scan_targets+=("$extra")
done

scanned=0; anomalies=0
if [ "${#scan_targets[@]}" -gt 0 ]; then
  while IFS= read -r f; do
    [ -n "$f" ] || continue
    scanned=$((scanned + 1))
    hits="$(grep -ohE '"(unsupported_opcodes|protocol_anomalies|residual_bytes)"[[:space:]]*:[[:space:]]*[0-9]+' "$f" 2>/dev/null \
      | tr -d ' \t' | awk -F: '($2+0)>0 {print $1" = "$2}' | sort | uniq -c | sed 's/^/  /')"
    words="$(grep -nE 'is not decoded|could not be decoded|left unwalked|desynchronis' "$f" 2>/dev/null | head -5 | sed 's/^/  /')"
    if [ -n "$hits" ] || [ -n "$words" ]; then
      anomalies=$((anomalies + 1))
      printf 'ANOMALY %s\n' "${f#"$REPO"/}" >>"$pl_log"
      [ -n "$hits" ]  && printf '%s\n' "$hits"  >>"$pl_log"
      [ -n "$words" ] && printf '%s\n' "$words" >>"$pl_log"
      printf '\n' >>"$pl_log"
    fi
  done < <(
    for t in "${scan_targets[@]}"; do
      if [ -d "$t" ]; then
        find "$t" -type f \( -name '*.json' -o -name '*.jsonl' -o -name '*.txt' -o -name '*.log' \)
      else
        printf '%s\n' "$t"
      fi
    done | sort -u )
fi
printf 'scanned %d file(s); %d carried anomalies\n' "$scanned" "$anomalies" >>"$pl_log"
if [ "$scanned" -eq 0 ]; then
  record protocol-log-check SKIP "no logs in scope; pass --logs PATH or --logs-all to widen" protocol-log-check
elif [ "$anomalies" -eq 0 ]; then
  record protocol-log-check PASS "$scanned log(s) scanned, no unsupported opcodes or residual bytes" protocol-log-check
else
  record protocol-log-check FAIL "$anomalies of $scanned log(s) carry protocol anomalies" protocol-log-check
fi

# ------------------------------------------- check: no secrets in the output ---
# A report that leaks a key is worse than no report. The needle is assembled at
# run time so this file never contains the literal it searches for, the same
# reason tests/secret_check.sh does it, and it is self-tested first because a
# scan that cannot match is indistinguishable from a clean result.
scan_log="$(logfile output-secret-scan)"
needle="$(printf -- '-----BEGIN [A-Z ]*PRIVATE%sKEY-----' ' ')"
cred='(PASSWORD|PASSWD|SECRET|TOKEN|APIKEY|API_KEY)[[:space:]]*[=:][[:space:]]*[A-Za-z0-9/+_-]{6,}'
if ! printf -- '-----BEGIN RSA PRIVATE%sKEY-----\n' ' ' | grep -qE -e "$needle"; then
  printf 'the pattern does not match PEM armour; this scan would be vacuous\n' >"$scan_log"
  record output-secret-scan FAIL "scan is vacuous; harness output left unverified" output-secret-scan
else
  leak="$( { grep -rlE -e "$needle" "$OUT" 2>/dev/null; grep -rlE "$cred" "$OUT" 2>/dev/null; } | sort -u )"
  if [ -n "$leak" ]; then
    printf 'carrying key material or credential assignments:\n%s\n' "$leak" >"$scan_log"
    record output-secret-scan FAIL "harness output carries secret-shaped content" output-secret-scan
  else
    printf 'no PEM armour and no credential assignments under %s (pattern self-tested)\n' "$OUT" >"$scan_log"
    record output-secret-scan PASS "logs carry no key material (pattern self-tested)" output-secret-scan
  fi
fi

# ------------------------------------------------------------------ report ---
n_pass=0; n_fail=0; n_skip=0
for s in "${CHECK_STATUS[@]}"; do
  case "$s" in
    PASS) n_pass=$((n_pass + 1)) ;;
    FAIL) n_fail=$((n_fail + 1)) ;;
    SKIP) n_skip=$((n_skip + 1)) ;;
  esac
done
overall="PASS"
[ "$n_skip" -gt 0 ] && overall="PASS_WITH_SKIPS"
[ "$n_fail" -gt 0 ] && overall="FAIL"

{
  printf '# REAL33D 3D regression harness\n\n'
  printf '**Overall: %s** — %d PASS, %d FAIL, %d SKIP.\n\n' "$overall" "$n_pass" "$n_fail" "$n_skip"
  printf 'A SKIP is not a pass. It records a check this run could not execute, and why.\n\n'

  printf '## Provenance\n\n| | |\n|---|---|\n'
  printf '| Run (UTC) | `%s` |\n' "$STAMP"
  printf '| Repository | `%s` |\n' "$REPO"
  printf '| Branch | `%s` |\n' "$BRANCH"
  printf '| HEAD | `%s` |\n' "$HEAD_SHA"
  printf '| HEAD subject | %s |\n' "$HEAD_SUBJECT"
  printf '| origin/main | `%s` |\n' "$ORIGIN_MAIN"
  printf '| Merge base | `%s` |\n' "${MERGE_BASE:-(none)}"
  printf '| Ahead / behind origin/main | %s / %s |\n' "$AHEAD" "$BEHIND"
  printf '| Uncommitted paths | %s |\n' "$DIRTY_COUNT"
  printf '| Toolchain | %s |\n\n' "$TOOLCHAIN_NOTE"

  if [ -s "$(logfile provenance-worktrees)" ]; then
    printf 'Worktrees at run time:\n\n```\n'; cat "$(logfile provenance-worktrees)"; printf '```\n\n'
  fi
  if [ "${DIRTY_COUNT:-0}" != "0" ] && [ -s "$(logfile provenance-status)" ]; then
    printf 'Uncommitted paths at run time:\n\n```\n'; cat "$(logfile provenance-status)"; printf '```\n\n'
  fi

  printf '## Results\n\n| Check | Status | Detail | Log |\n|---|---|---|---|\n'
  i=0
  while [ "$i" -lt "${#CHECK_NAMES[@]}" ]; do
    lg="${CHECK_LOG[$i]}"
    if [ -n "$lg" ]; then lg="\`logs/$lg.txt\`"; else lg="—"; fi
    printf '| `%s` | **%s** | %s | %s |\n' \
      "${CHECK_NAMES[$i]}" "${CHECK_STATUS[$i]}" "${CHECK_SUMMARY[$i]}" "$lg"
    i=$((i + 1))
  done
  printf '\n'

  printf '## What each check covers\n\n'
  printf -- '- `provenance` — branch, HEAD, origin/main, merge base, ahead/behind, dirty paths and the full worktree list, captured before anything else runs.\n'
  printf -- '- `branch-isolation` — refuses to certify a run sitting on `main`.\n'
  printf -- '- `clientcore-tests` — the Protocol772Core CTest suites: transport, crypto, login, gamelogin, initial_world, movement, player_state, worldview, minimap.\n'
  printf -- '- `real33d-native-tests` — the header-only 3D unit tests under `tests/real33d_*.cpp`: mouse gestures, minimap view, item-use/MultiUse classification.\n'
  printf -- '- `secret-check` — `tests/secret_check.sh` over the tracked tree and the whole reachable history.\n'
  printf -- '- `build-sanity-msvc` — `tests/build_clientcore_windows.cmd`: the C++17 suites plus the C++20 archive the Unreal module links.\n'
  printf -- '- `unreal-module-build` — UnrealBuildTool on REAL33DEditor. Opt-in with `--unreal`.\n'
  printf -- '- `protocol-log-check` — `unsupported_opcodes`, `protocol_anomalies` and `residual_bytes` above zero, plus the bridge diagnostics that say a command was left unwalked.\n'
  printf -- '- `output-secret-scan` — the harness re-reads everything it just wrote and fails rather than leave key material or a credential assignment on disk.\n\n'

  printf '## Scope and limits\n\n'
  printf -- '- Nothing here proves live behaviour. No Fusion32 server is started, no client is launched, no gameplay and no protocol is modified. Live certification stays a separate, operator-driven step.\n'
  printf -- '- Read-only against the repository. Everything written goes under `%s`, inside the gitignored `build/` tree.\n' "${OUT#"$REPO"/}"
  printf -- '- No environment values are printed, and `output-secret-scan` is the last word on what was left on disk.\n'
} >"$REPORT"

# Last word on what is left on disk. The report is written after the scan
# above, so it gets its own pass; if it trips, the content is replaced rather
# than published, because a leaked report cannot be un-read.
if grep -qE -e "$needle" "$REPORT" 2>/dev/null || grep -qE "$cred" "$REPORT" 2>/dev/null; then
  printf '# REAL33D 3D regression harness\n\nReport withheld: the generated text matched the key or credential pattern and was discarded. See logs/ and re-run after removing the source of the match.\n' >"$REPORT"
  printf 'the generated report matched the secret pattern and was discarded\n' >>"$scan_log"
  printf '\n%-22s %s\n' "OVERALL" "FAIL (report withheld)"
  exit 1
fi

printf '\n%-22s %s\n' "OVERALL" "$overall"
printf 'report: %s\n' "$REPORT"

if [ "$n_fail" -gt 0 ]; then exit 1; fi
if [ "$STRICT" -eq 1 ] && [ "$n_skip" -gt 0 ]; then exit 1; fi
exit 0
