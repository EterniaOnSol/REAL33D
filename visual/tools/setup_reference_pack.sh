#!/usr/bin/env bash
# One-command setup for the REAL33D artist reference pack.
#
# Rebuilds the master inventory and then the navigable catalogue with previews.
# Run it after cloning, once the authorised Tibia 7.72 client data is in place.
#
#   bash visual/tools/setup_reference_pack.sh [repo-root]
#
# Everything it writes under visual/reference_pack/ is gitignored, because the
# previews are derived from a client artifact whose provenance is UNKNOWN.
set -uo pipefail

root="$(cd "${1:-$(dirname "$0")/../..}" && pwd)"
cd "$root"

archive="$root/tibia-game.tarball.tar.gz"
dat="$root/build/classic-client-772/app/Tibia.dat"
spr="$root/build/classic-client-772/app/Tibia.spr"
manifests="$root/visual/manifests"
pack="$root/visual/reference_pack"

fail=0
note() { printf '%s\n' "$1"; }
missing() { printf 'MISSING: %s\n' "$1" >&2; fail=1; }

note "REAL33D reference pack setup"
note "repository: $root"
note ""

note "checking inputs"
[ -f "$archive" ] || missing "$archive (historical runtime archive, needed for the master inventory)"
[ -f "$dat" ] || missing "$dat (Tibia 7.72 client appearance data)"
[ -f "$spr" ] || missing "$spr (Tibia 7.72 client sprite data)"

if [ "$fail" -ne 0 ]; then
  cat >&2 <<'EOF'

The visual inputs are not redistributed with this repository. Place the
authorised Tibia 7.72 client files at:

    build/classic-client-772/app/Tibia.dat
    build/classic-client-772/app/Tibia.spr

then run this script again. Expected SHA-256, recorded in
evidence/client/CLASSIC-CLIENT-772-001.md:

    Tibia.dat  3C5E857FF72FD1E52879EB8845ADC72C0991786AD0EAF85F6847595C9677AEDD
    Tibia.spr  86ABBF5FADCF84313A03615A55A8BD626BAAB98671F0A78C9B36CB61CCE0C64A

Do not substitute another version or a download from elsewhere: the catalogue
would silently describe a different game.
EOF
  exit 2
fi

note "verifying the visual inputs against the recorded hashes"
expected_dat="3c5e857ff72fd1e52879eb8845adc72c0991786ad0eaf85f6847595c9677aedd"
expected_spr="86abbf5fadcf84313a03615a55a8bd626baab98671f0a78c9b36cb61cce0c64a"
actual_dat="$(sha256sum "$dat" | awk '{print tolower($1)}')"
actual_spr="$(sha256sum "$spr" | awk '{print tolower($1)}')"
if [ "$actual_dat" != "$expected_dat" ] || [ "$actual_spr" != "$expected_spr" ]; then
  printf 'HASH MISMATCH. These are not the client files this project was built against.\n' >&2
  printf '  Tibia.dat expected %s\n            got      %s\n' "$expected_dat" "$actual_dat" >&2
  printf '  Tibia.spr expected %s\n            got      %s\n' "$expected_spr" "$actual_spr" >&2
  exit 3
fi
note "  both match"

note ""
note "1/2 regenerating the master inventory"
python3 "$root/visual/tools/extract_visual_inventory.py" \
  --archive "$archive" --source "$root/reference/game/src" --out "$manifests" || exit 4
python3 "$root/visual/tools/sync_tracker.py" \
  --manifests "$manifests" \
  --tracker "$root/visual/tracker/VISUAL_TRACKER.csv" \
  --p0-out "$root/visual/rookgaard_p0/P0_ASSETS.csv" || exit 5

note ""
note "2/2 building the reference pack"
python3 "$root/visual/tools/build_reference_pack.py" \
  --dat "$dat" --spr "$spr" --manifests "$manifests" --out "$pack" \
  --scale 3 || exit 6

note ""
note "done. open these in a browser:"
note "  $pack/catalogue.html      the whole catalogue"
note "  $pack/p0_rookgaard.html   the Rookgaard P0 queue to start from"
