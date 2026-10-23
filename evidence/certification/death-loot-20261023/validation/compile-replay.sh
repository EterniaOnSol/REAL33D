#!/usr/bin/env bash
set -euo pipefail
cd /mnt/c/Users/dell/Desktop/fusion32/build/cert-death-loot-20261023
g++ -std=c++17 -O0 -Iclientcore/include -Iunreal/REAL33D/Source/REAL33D/Public \
 evidence/certification/death-loot-20261023/replay.cpp clientcore/src/*.cpp \
 -lcrypto -pthread -o build/certification/replay
