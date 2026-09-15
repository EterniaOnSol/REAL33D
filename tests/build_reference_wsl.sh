#!/usr/bin/env bash
set -euo pipefail

workspace="${1:?usage: build_reference_wsl.sh /mnt/c/path/to/fusion32}"
build_root="$(mktemp -d /tmp/fusion32-bootstrap-build-001.XXXXXX)"

cp -a "$workspace/reference/game" "$build_root/game"
cp -a "$workspace/reference/login" "$build_root/login"
cp -a "$workspace/reference/querymanager" "$build_root/querymanager"

game_flags='-m64 -fno-strict-aliasing -pedantic -Wall -Wextra -Wno-deprecated-declarations -Wno-unused-parameter -Wno-format-truncation -std=c++11 -pthread -DOS_LINUX=1 -DARCH_X64=1 -DTIBIA772=1 -O2'
login_flags='-m64 -fno-strict-aliasing -Wno-deprecated-declarations -pedantic -Wall -Wextra -pthread --std=c++11 -DTIBIA772=1 -O2'

make -C "$build_root/game" -B DEBUG=0 CFLAGS="$game_flags" >"$build_root/game-build.log" 2>&1
printf 'GAME_EXIT=0\n'
sha256sum "$build_root/game/build/game"

make -C "$build_root/login" -B DEBUG=0 CXXFLAGS="$login_flags" >"$build_root/login-build.log" 2>&1
printf 'LOGIN_EXIT=0\n'
sha256sum "$build_root/login/build/login"

make -C "$build_root/querymanager" -B DEBUG=0 DATABASE=sqlite >"$build_root/querymanager-build.log" 2>&1
printf 'QUERYMANAGER_EXIT=0\n'
sha256sum "$build_root/querymanager/build/querymanager"

printf 'BUILD_ROOT=%s\n' "$build_root"
wc -l "$build_root"/*-build.log
