#!/usr/bin/env bash
set -euo pipefail
export PATH="/c/Program Files (x86)/Microsoft Visual Studio/2022/BuildTools/VC/Tools/MSVC/14.44.35207/bin/Hostx64/x64:$PATH"
bash tests/regression_harness.sh --out build/certification/harness --objects-srv /var/lib/fusion32-server-baseline-772-0/game/reference/dat/objects.srv --unreal --strict --logs evidence/clientcore/unreal-chat-channels-independent-20261018/live
