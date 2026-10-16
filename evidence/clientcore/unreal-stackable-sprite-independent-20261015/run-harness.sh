#!/usr/bin/env bash
export PATH="/c/Program Files (x86)/Microsoft Visual Studio/2026/BuildTools/VC/Tools/MSVC/14.51.36231/bin/Hostx64/x64:$PATH"
exec bash tests/regression_harness.sh --out build/certification/harness-closeout --strict --unreal --objects-srv /var/lib/fusion32-server-baseline-772-0/game/reference/dat/objects.srv --logs build/unreal-world-presentation-polish-001/live-20261015T204404
