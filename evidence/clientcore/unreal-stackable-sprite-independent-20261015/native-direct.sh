#!/usr/bin/env bash
set -euo pipefail
cd /mnt/c/Users/dell/Desktop/fusion32/build/cert-stackable-20261015
out=build/certification/native-direct
mkdir -p "$out"
for name in mouse_input minimap_view item_use_policy stackable_sprite; do
  stem="real33d_${name}_tests"
  g++ -std=c++17 -Wall -Wextra -Wpedantic -Werror -I unreal/REAL33D/Source/REAL33D/Public "tests/$stem.cpp" -o "$out/$stem"
  if [[ "$name" == item_use_policy ]]; then
    "$out/$stem" /var/lib/fusion32-server-baseline-772-0/game/reference/dat/objects.srv
  else
    "$out/$stem"
  fi
done
