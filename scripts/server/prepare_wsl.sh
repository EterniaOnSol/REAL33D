#!/usr/bin/env bash
set -euo pipefail

workspace="$(realpath "${1:?usage: prepare_wsl.sh /mnt/c/path/to/fusion32}")"
runtime="/tmp/fusion32-server-baseline-772-${UID}"
archive="$workspace/tibia-game.tarball.tar.gz"
umask 077

expected_archive_sha256='67b771d1e3b4a6ef48c554b9b8b0db56da39cae6b0de5444f7bf6e71c0b2de8e'
expected_data_manifest_sha256='bb7f3dc393d8686ed89b3949d7fc5f6086d7b49810b54b7f9403e560b2e7acb5'
expected_game_sha256='e0e96b863fb3fa061501e1eb59c6bac456733676a15e11f245c6b04505e817c7'
expected_login_sha256='23f0e75536a852908da35e9644785f2e47101948e723fa426bf0fea453be788f'
expected_querymanager_sha256='50474952c55c4df3ef3b06c08b475550326e5aba9d8ad8b3502af401913625e4'

require_sha256() {
  local file="$1" expected="$2" label="$3" actual
  actual="$(sha256sum "$file" | awk '{print $1}')"
  if [[ "$actual" != "$expected" ]]; then
    printf '%s SHA-256 mismatch: expected %s, got %s\n' "$label" "$expected" "$actual" >&2
    exit 3
  fi
}

if [[ -e "$runtime" ]]; then
  echo "Refusing to overwrite existing runtime: $runtime" >&2
  exit 2
fi
test -f "$archive"
test -f "$workspace/reference/game/Makefile"
test -f "$workspace/reference/login/Makefile"
test -f "$workspace/reference/querymanager/Makefile"
require_sha256 "$archive" "$expected_archive_sha256" 'historical runtime archive'

mkdir -p "$runtime"/{build,game/{bin,reference,state/{map,usr,save},logs,run},login/{bin,logs,run},querymanager/{bin,state,logs,run,sqlite/patches},secrets}
chmod 700 "$runtime" "$runtime/secrets" "$runtime/game" "$runtime/login" "$runtime/querymanager"

game_flags='-m64 -fno-strict-aliasing -pedantic -Wall -Wextra -Wno-deprecated-declarations -Wno-unused-parameter -Wno-format-truncation -std=c++11 -pthread -DOS_LINUX=1 -DARCH_X64=1 -DTIBIA772=1 -O2'
login_flags='-m64 -fno-strict-aliasing -Wno-deprecated-declarations -pedantic -Wall -Wextra -pthread --std=c++11 -DTIBIA772=1 -O2'

make -C "$workspace/reference/game" -B DEBUG=0 BUILDDIR="$runtime/build/game" CFLAGS="$game_flags" >"$runtime/build/game.log" 2>&1
make -C "$workspace/reference/login" -B DEBUG=0 BUILDDIR="$runtime/build/login" CXXFLAGS="$login_flags" >"$runtime/build/login.log" 2>&1
make -C "$workspace/reference/querymanager" -B DEBUG=0 DATABASE=sqlite BUILDDIR="$runtime/build/querymanager" >"$runtime/build/querymanager.log" 2>&1

install -m 755 "$runtime/build/game/game" "$runtime/game/bin/game"
install -m 755 "$runtime/build/login/login" "$runtime/login/bin/login"
install -m 755 "$runtime/build/querymanager/querymanager" "$runtime/querymanager/bin/querymanager"
require_sha256 "$runtime/game/bin/game" "$expected_game_sha256" 'Game binary'
require_sha256 "$runtime/login/bin/login" "$expected_login_sha256" 'Login binary'
require_sha256 "$runtime/querymanager/bin/querymanager" "$expected_querymanager_sha256" 'Query Manager binary'

tar -xzf "$archive" -C "$runtime/game/reference" \
  ./dat/circles.dat ./dat/map.dat ./dat/mem.dat ./dat/houseareas.dat \
  ./dat/objects.srv ./dat/conversion.lst ./dat/houses.dat \
  ./dat/monster.db ./dat/moveuse.dat ./origmap ./npc ./mon

cp -a "$runtime/game/reference/origmap/." "$runtime/game/state/map/"
for n in $(seq -w 0 99); do mkdir -p "$runtime/game/state/usr/$n"; done

cp "$workspace/reference/querymanager/sqlite/schema.sql" "$runtime/querymanager/sqlite/schema.sql"

openssl genrsa -traditional -out "$runtime/secrets/tibia.pem" 1024 >/dev/null 2>&1
chmod 600 "$runtime/secrets/tibia.pem"
openssl rsa -in "$runtime/secrets/tibia.pem" -check -noout >/dev/null 2>&1
install -m 600 "$runtime/secrets/tibia.pem" "$runtime/game/tibia.pem"
install -m 600 "$runtime/secrets/tibia.pem" "$runtime/login/tibia.pem"
openssl rsa -in "$runtime/secrets/tibia.pem" -pubout -outform DER 2>/dev/null | sha256sum | awk '{print $1}' >"$runtime/secrets/public-key.sha256"
openssl rsa -in "$runtime/secrets/tibia.pem" -noout -modulus 2>/dev/null | sed 's/^Modulus=//' >"$runtime/secrets/public-modulus.hex"

python3 - "$runtime" <<'PY'
import hashlib
import os
import secrets
import string
import sys
from pathlib import Path

runtime = Path(sys.argv[1])
alphabet = string.ascii_letters + string.digits
key = b"Pm-,o%yD"

modulus_hex = (runtime / 'secrets' / 'public-modulus.hex').read_text(encoding='ascii').strip()
(runtime / 'secrets' / 'public-modulus.decimal').write_text(
    f"{int(modulus_hex, 16)}\n", encoding='ascii')

def token(n):
    return ''.join(secrets.choice(alphabet) for _ in range(n))

def disguise(value):
    raw = value.encode('ascii')
    return ''.join(chr((key[i] - raw[i] + 0x5E) % 0x5E + 0x21) for i in range(len(raw)))

while True:
    qm_password = token(8)
    disguised = disguise(qm_password)
    if all(ch not in '"\\' and 0x21 <= ord(ch) <= 0x7E for ch in disguised):
        break

account_a_password = token(16)
account_b_password = token(16)

def auth_hex(password):
    salt = os.urandom(32)
    inner = hashlib.sha256(password.encode('ascii')).digest()
    mixed = bytes(a ^ b for a, b in zip(inner, salt))
    return (hashlib.sha256(mixed).digest() + salt).hex()

auth_a = auth_hex(account_a_password)
auth_b = auth_hex(account_b_password)

(runtime / 'secrets' / 'credentials.env').write_text(
    f"QM_PASSWORD={qm_password}\n"
    f"ACCOUNT_A_ID=772001\nACCOUNT_A_PASSWORD={account_a_password}\nCHARACTER_A=Test Player A\n"
    f"ACCOUNT_B_ID=772002\nACCOUNT_B_PASSWORD={account_b_password}\nCHARACTER_B=Test Player B\n",
    encoding='utf-8')
os.chmod(runtime / 'secrets' / 'credentials.env', 0o600)

(runtime / 'querymanager' / 'config.cfg').write_text(
    'SQLite.File = "state/tibia.db"\n'
    'SQLite.MaxCachedStatements = 100\n'
    'QueryManagerPort = 7173\n'
    f'QueryManagerPassword = "{qm_password}"\n'
    'QueryWorkerThreads = 1\nQueryBufferSize = 1M\nQueryMaxAttempts = 3\n'
    'MaxConnections = 25\nMaxConnectionIdleTime = 5m\n', encoding='utf-8')

(runtime / 'login' / 'config.cfg').write_text(
    'LoginPort = 7171\nConnectionTimeout = 5s\nMaxConnections = 10\n'
    'MaxStatusRecords = 1024\nMinStatusInterval = 5m\n'
    'QueryManagerHost = "127.0.0.1"\nQueryManagerPort = 7173\n'
    f'QueryManagerPassword = "{qm_password}"\n'
    'StatusWorld = "Fusion Test"\nURL = ""\nLocation = "Local Test"\n'
    'ServerType = "Tibia"\nServerVersion = "7.72"\nClientVersion = "7.72"\n'
    'MOTD = "Fusion32 sanitized test runtime"\n', encoding='utf-8')

g = runtime / 'game'
(g / '.tibia').write_text(
    f'binpath = "{g / "bin"}"\n'
    f'mappath = "{g / "state" / "map"}"\n'
    f'origmappath = "{g / "reference" / "origmap"}"\n'
    f'datapath = "{g / "reference" / "dat"}"\n'
    f'monsterpath = "{g / "reference" / "mon"}"\n'
    f'npcpath = "{g / "reference" / "npc"}"\n'
    f'userpath = "{g / "state" / "usr"}"\n'
    f'logpath = "{g / "logs"}"\n'
    f'savepath = "{g / "state" / "save"}"\n'
    'shm = 77201\nadminport = 0\nadminaddress = "127.0.0.1"\n'
    'debuglevel = 3\nstate = public\nworld = "Fusion Test"\nbeat = 50\n'
    f'querymanager = {{("127.0.0.1",7173,"{disguised}")}}\n', encoding='utf-8')

seed = f'''INSERT INTO Worlds (WorldID,Name,Type,RebootTime,Host,Port,MaxPlayers,PremiumPlayerBuffer,MaxNewbies,PremiumNewbieBuffer)
VALUES (1,'Fusion Test',0,5,'127.0.0.1',7172,100,10,100,10);
INSERT INTO Accounts(AccountID,Email,Auth) VALUES
(772001,'test-a@invalid.local',X'{auth_a}'),
(772002,'test-b@invalid.local',X'{auth_b}');
INSERT INTO Characters(WorldID,CharacterID,AccountID,Name,Sex) VALUES
(1,1001,772001,'Test Player A',1),
(1,1002,772002,'Test Player B',2);
'''
(runtime / 'querymanager' / 'sqlite' / 'patches' / '000-local-synthetic-seed.sql').write_text(seed, encoding='utf-8')
PY

chmod 600 "$runtime/game/.tibia" "$runtime/login/config.cfg" "$runtime/querymanager/config.cfg" "$runtime/querymanager/sqlite/patches/000-local-synthetic-seed.sql"

(
  cd "$runtime/game/reference"
  find dat origmap npc mon -type f -print0 | sort -z | xargs -0 sha256sum
) >"$runtime/data-provenance.sha256"
require_sha256 "$runtime/data-provenance.sha256" "$expected_data_manifest_sha256" 'runtime data manifest'

sha256sum "$runtime/game/bin/game" "$runtime/login/bin/login" "$runtime/querymanager/bin/querymanager" >"$runtime/build/binaries.sha256"
printf 'Prepared sanitized runtime at %s\n' "$runtime"
printf 'Public key fingerprint: %s\n' "$(cat "$runtime/secrets/public-key.sha256")"
