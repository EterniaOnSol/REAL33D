#!/usr/bin/env bash
set -euo pipefail

workspace="$(realpath "${1:?usage: runtime_smoke_wsl.sh /mnt/c/path/to/fusion32}")"
runtime="/tmp/fusion32-server-baseline-772-${UID}"

expected_data_manifest_sha256='bb7f3dc393d8686ed89b3949d7fc5f6086d7b49810b54b7f9403e560b2e7acb5'
expected_game_sha256='e0e96b863fb3fa061501e1eb59c6bac456733676a15e11f245c6b04505e817c7'
expected_login_sha256='23f0e75536a852908da35e9644785f2e47101948e723fa426bf0fea453be788f'
expected_querymanager_sha256='50474952c55c4df3ef3b06c08b475550326e5aba9d8ad8b3502af401913625e4'

assert_mode() {
  local expected="$1" path="$2" actual
  actual="$(stat -c '%a' "$path")"
  [[ "$actual" = "$expected" ]] || { echo "Unsafe mode $actual on $path; expected $expected" >&2; exit 1; }
}

bash "$workspace/scripts/server/network_smoke_wsl.sh" "$workspace"

(
  cd "$runtime/game/reference"
  sha256sum --quiet -c "$runtime/data-provenance.sha256"
)
(
  cd "$runtime"
  sha256sum --quiet -c build/binaries.sha256
)
[[ "$(sha256sum "$runtime/data-provenance.sha256" | awk '{print $1}')" = "$expected_data_manifest_sha256" ]]
[[ "$(sha256sum "$runtime/game/bin/game" | awk '{print $1}')" = "$expected_game_sha256" ]]
[[ "$(sha256sum "$runtime/login/bin/login" | awk '{print $1}')" = "$expected_login_sha256" ]]
[[ "$(sha256sum "$runtime/querymanager/bin/querymanager" | awk '{print $1}')" = "$expected_querymanager_sha256" ]]
cmp -s "$runtime/secrets/tibia.pem" "$runtime/game/tibia.pem"
cmp -s "$runtime/secrets/tibia.pem" "$runtime/login/tibia.pem"
assert_mode 700 "$runtime"
assert_mode 700 "$runtime/secrets"
for secret in \
  "$runtime/secrets/tibia.pem" \
  "$runtime/secrets/credentials.env" \
  "$runtime/game/tibia.pem" \
  "$runtime/game/.tibia" \
  "$runtime/login/tibia.pem" \
  "$runtime/login/config.cfg" \
  "$runtime/querymanager/config.cfg" \
  "$runtime/querymanager/sqlite/patches/000-local-synthetic-seed.sql" \
  "$runtime/querymanager/state/tibia.db"; do
  assert_mode 600 "$secret"
done

python3 - "$runtime/querymanager/state/tibia.db" <<'PY'
import sqlite3
import sys

database = sys.argv[1]
connection = sqlite3.connect(f"file:{database}?mode=ro", uri=True)

assert connection.execute("SELECT COUNT(*) FROM Worlds").fetchone()[0] == 1
assert connection.execute("SELECT COUNT(*) FROM Accounts").fetchone()[0] == 2
assert connection.execute("SELECT COUNT(*) FROM Characters").fetchone()[0] == 2
assert connection.execute(
    "SELECT Name, Host, Port FROM Worlds"
).fetchone() == ("Fusion Test", "127.0.0.1", 7172)
assert connection.execute(
    "SELECT Name FROM Characters ORDER BY CharacterID"
).fetchall() == [("Test Player A",), ("Test Player B",)]
print("SANITIZED_DB worlds=1 accounts=2 characters=2 PASS")
PY

sectors="$(grep -Eo '^[0-9]+ Sektoren geladen\.$' "$runtime/game/logs/game.log" | tail -n 1 | cut -d' ' -f1)"
objects="$(grep -Eo '^[0-9]+ Objekte geladen\.$' "$runtime/game/logs/game.log" | tail -n 1 | cut -d' ' -f1)"
test "$sectors" = 9873
test "$objects" = 8533464
printf 'GAME_WORLD sectors=%s objects=%s PASS\n' "$sectors" "$objects"
echo 'RSA_SHARED_PRIVATE_MATERIAL_MATCH PASS'
echo 'SECRET_ACCESS runtime=700 sensitive_files=600 PASS'
echo 'SERVER-RUNTIME-SMOKE-001 PASS'
