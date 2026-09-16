#!/usr/bin/env bash
set -euo pipefail

workspace="$(realpath "${1:?usage: prepare_fusion32_ipchanger_wsl.sh /mnt/c/path/to/fusion32}")"
runtime="/tmp/fusion32-server-baseline-772-${UID}"
output="$workspace/build/ipchanger/servers.txt"

test -f "$runtime/secrets/public-modulus.decimal"
modulus="$(cat "$runtime/secrets/public-modulus.decimal")"
[[ "$modulus" =~ ^[0-9]+$ ]]
(( ${#modulus} + 1 <= 312 ))

mkdir -p "$(dirname "$output")"
printf '772;fusion32;127.0.0.1;7171;%s\n' "$modulus" >"$output"
printf 'Prepared Fusion32 IP Changer entry at %s (modulus characters: %d).\n' "$output" "${#modulus}"
