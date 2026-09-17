#!/usr/bin/env bash
# Secret and sensitive-file check, run before every push.
#
# Scans the tracked tree and the whole reachable history for private keys,
# credentials and runtime secrets, and verifies that reference/ has not been
# modified outside its baseline commits.
#
# Usage: tests/secret_check.sh [repo-root]
set -uo pipefail

root="$(cd "${1:-$(dirname "$0")/..}" && pwd)"
cd "$root"
failures=0

fail() { printf 'FAIL: %s\n' "$1" >&2; failures=$((failures + 1)); }
pass() { printf 'ok   : %s\n' "$1"; }

# 1. No secret-bearing file may be tracked.
hits="$(git ls-files \
  | grep -iE '\.(pem|key|p12|pfx|jks|ppk)$|(^|/)(config\.cfg|credentials\.env|id_rsa|\.env)$' \
  || true)"
if [ -n "$hits" ]; then
  fail "secret-bearing files are tracked:"$'\n'"$hits"
else
  pass "no secret-bearing file extensions tracked"
fi

# 2. Runtime and scratch trees must never be tracked.
hits="$(git ls-files | grep -E '^(runtime/|reference/runtime/|runtime-legacy/|\.audit/|\.agents/|build/)' || true)"
if [ -n "$hits" ]; then
  fail "runtime or scratch paths are tracked:"$'\n'"$hits"
else
  pass "runtime and scratch trees untracked"
fi

# 3. No private key material anywhere in reachable history.
#    Real key material is always PEM-armoured, so the pattern requires the
#    surrounding dashes. That is both more precise than a bare phrase match and
#    the reason this script cannot flag its own committed copies, which mention
#    the phrase without the armour. The needle is still assembled at run time so
#    the literal never appears here either.
#    The pattern starts with dashes, so it must be passed with -e or git grep
#    parses it as an option.
needle="$(printf -- '-----BEGIN [A-Z ]*PRIVATE%sKEY-----' ' ')"

# Self-test: a check that cannot detect a planted key is worse than no check.
if ! printf -- '-----BEGIN RSA PRIVATE%sKEY-----\n' ' ' | grep -qE -e "$needle"; then
  fail "the private key pattern does not match PEM armour; the scan is vacuous"
else
  hits="$(git grep -I -l -E -e "$needle" $(git rev-list --all) -- 2>/dev/null | head -5 || true)"
  if [ -n "$hits" ]; then
    fail "private key material found in history:"$'\n'"$hits"
  else
    pass "no private key blocks in any reachable commit (pattern self-tested)"
  fi
fi

# 4. No generated runtime credential values in the tracked tree. The
#    preparation script only ever writes variable names; the values it
#    generates land in the gitignored runtime.
hits="$(git grep -nIE '(ACCOUNT_[AB]_PASSWORD|QM_PASSWORD)=[A-Za-z0-9]{6,}' -- ':!scripts/' || true)"
if [ -n "$hits" ]; then
  fail "generated credential values are tracked:"$'\n'"$hits"
else
  pass "no generated credential values tracked"
fi

# 5. reference/ is archived third-party source. Only the baseline commits may
#    have touched it.
allowed='9d013c1|e7febfc|d6201d2'
hits="$(git log --format='%h' -- reference/ | grep -vE "^($allowed)" || true)"
if [ -n "$hits" ]; then
  fail "reference/ modified outside its baseline commits: $hits"
else
  pass "reference/ untouched outside its baseline commits"
fi

# 6. Reviewed and accepted: the archived upstream service templates carry
#    Fusion32's own default query-manager password. They are third-party source
#    material, not a secret of this deployment, whose runtime config.cfg is
#    generated fresh and gitignored.
if git ls-files | grep -q 'reference/login/config.cfg.dist'; then
  pass "archived upstream config templates present and accepted (see evidence)"
fi

if [ "$failures" -eq 0 ]; then
  echo "secret_check: PASS"
  exit 0
fi
echo "secret_check: FAIL ($failures)" >&2
exit 1
