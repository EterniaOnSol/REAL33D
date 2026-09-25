#!/usr/bin/env bash
# Launch the opt-in REAL33D2D agent against a prepared local Fusion32 QA runtime.
#
# No deployment path, character or credential is hardcoded. Local settings come
# from agent.local.env (gitignored; see agent.local.env.example) or from the
# environment. Credentials are read from the QA runtime's own generated secrets
# file into this process only: never echoed, never passed as arguments, never
# written anywhere by this script.
set -euo pipefail

here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if [[ -f "$here/agent.local.env" ]]; then
  set -a
  # shellcheck disable=SC1091
  . "$here/agent.local.env"
  set +a
fi

require() {
  local name="$1"
  if [[ -z "${!name:-}" ]]; then
    echo "$name is not set. Copy agent.local.env.example to agent.local.env and fill it in." >&2
    exit 1
  fi
}
require R33D_CLIENT_ROOT
require R33D_WSL_DISTRO
require R33D_QA_CREDENTIALS
require R33D_QA_CHARACTER

mode="${R33D_AGENT_RUN_MODE:-bridge}"
brain="${R33D_AGENT_BRAIN:-mock}"
if [[ "$mode" == "bridge" && "$brain" != "mock" ]]; then
  echo 'Bridge certification is mock-only. Set R33D_AGENT_RUN_MODE=legacy for another brain.' >&2
  exit 1
fi
if [[ ! "$R33D_QA_CHARACTER" =~ ^[A-Za-z0-9_]+$ ]]; then
  echo 'R33D_QA_CHARACTER must be alphanumeric' >&2
  exit 1
fi

cred="$(MSYS_NO_PATHCONV=1 timeout 60 wsl.exe -d "$R33D_WSL_DISTRO" -- cat "$R33D_QA_CREDENTIALS" | tr -d '\r')"
R33D_ACC="$(printf '%s\n' "$cred" | awk -F= -v k="ACCOUNT_${R33D_QA_CHARACTER}_ID=" 'index($0, k)==1{print $2}')"
R33D_PW="$(printf '%s\n' "$cred" | awk -F= -v k="ACCOUNT_${R33D_QA_CHARACTER}_PASSWORD=" 'index($0, k)==1{print $2}')"
unset cred
if [[ -z "$R33D_ACC" || -z "$R33D_PW" ]]; then
  echo 'Agent launch failed: QA credential fields missing' >&2
  exit 1
fi
export R33D_ACC R33D_PW
export R33D_AGENT_AUTOLOGIN=1

if [[ "$mode" == "bridge" ]]; then
  export R33D_AGENT_MODE=1
  export R33D_AGENT_BRAIN=mock
  export R33D_AGENT_TRACE="${R33D_AGENT_TRACE:-$R33D_CLIENT_ROOT/real33d_agent_trace.jsonl}"
  echo "mode=bridge brain=mock trace=$R33D_AGENT_TRACE"
else
  export R33D_AGENT=1
  export R33D_AGENT_BRAIN="$brain"
  echo "mode=legacy brain=$brain"
fi

unset R33D_ACCEPTANCE R33D_MOVEONLY R33D_TAPTEST R33D_MANUALTAP
cd "$R33D_CLIENT_ROOT"
exec ./Release/otclient.exe
