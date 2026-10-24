#!/usr/bin/env bash
set -euo pipefail
cd /mnt/c/Users/dell/Desktop/fusion32/build/cert-player-death-20261023
build/certification/replay /var/lib/fusion32-server-baseline-772-0/game/reference/dat/objects.srv build/certification/live/packets.tsv > build/certification/live/replayed.jsonl
