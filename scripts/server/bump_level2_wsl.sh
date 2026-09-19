#!/usr/bin/env bash
set -euo pipefail

# Raises Test Player A and Test Player B to level 2 in their saved character
# files.
#
# Why this exists: `receiving.cc::CTalk` refuses TALK_YELL below level 2, so a
# level-1 character cannot exercise the yell path at all. The two synthetic test
# characters are created by the seed in `prepare_wsl.sh` and are born at level 1.
#
# This is TEST DATA, not 7.72 parity. Fusion32 never grants a level this way to
# an ordinary character; the one place it does is `crplayer.cc:162-168`, where a
# character holding GAMEMASTER_OUTFIT is raised on its first login. The values
# written here are copied from exactly those four lines, so the result is a
# state the server itself produces rather than one invented here:
#
#     Act = 2    Exp = 100    LastLevel = 100    NextLevel = 200
#
# LastLevel is deliberately absent from the file. `TSkill::Load` at
# `crskill.cc:106` recomputes it as `GetExpForLevel(Act)`, which is 100 for
# level 2, so writing Act, Exp and NextLevel is sufficient and consistent.
#
# The file format is the 15-field Skill line written by `crplayer.cc:2492-2522`:
#
#     Skill = (SkillNr,Actual,Maximum,Minimum,DeltaAct,MagicDeltaAct,Cycle,
#              MaxCycle,Count,MaxCount,AddLevel,Experience,FactorPercent,
#              NextLevel,Delta)
#
# SkillNr 0 is SKILL_LEVEL. Only fields 2, 12 and 14 are touched.
#
# THE SERVER MUST BE STOPPED. Game caches players in memory and writes them on
# shutdown, so an edit applied while it runs is read by nobody and is then
# overwritten by the cached copy.
#
#   scripts/server/bump_level2_wsl.sh

runtime="/var/lib/fusion32-server-baseline-772-${UID}"

for port in 7171 7172 7173; do
  if ss -ltnH "sport = :$port" | grep -q .; then
    echo "Refusing to edit save data while the server is running (port $port is listening)." >&2
    echo "Stop it first: scripts/server/stop_wsl.sh <workspace>" >&2
    exit 2
  fi
done

bump_one() {
  local file="$1" name="$2"

  if [[ ! -f "$file" ]]; then
    echo "MISSING: $file" >&2
    echo "$name has never logged in, so the server has not written a save file yet." >&2
    return 1
  fi

  local before
  before="$(grep -m1 '^Skill = (0,' "$file" || true)"
  if [[ -z "$before" ]]; then
    echo "No SKILL_LEVEL line in $file" >&2
    return 1
  fi

  cp -p "$file" "$file.bak-before-level-bump"

  awk '
    /^Skill = \(0,/ {
      # Strip "Skill = (" and the trailing ")", rewrite three fields, rebuild.
      line = $0
      sub(/^Skill = \(/, "", line)
      sub(/\)$/, "", line)
      n = split(line, f, ",")
      if (n != 15) { print "unexpected field count: " n > "/dev/stderr"; exit 1 }
      f[2]  = 2     # Actual    -> level 2
      f[12] = 100   # Experience-> GetExpForLevel(2)
      f[14] = 200   # NextLevel -> GetExpForLevel(3)
      out = f[1]
      for (i = 2; i <= n; i++) out = out "," f[i]
      print "Skill = (" out ")"
      next
    }
    { print }
  ' "$file" >"$file.tmp"

  mv "$file.tmp" "$file"
  chmod 600 "$file"

  printf '%s\n  before: %s\n  after:  %s\n' \
    "$name" "$before" "$(grep -m1 '^Skill = (0,' "$file")"
}

bump_one "$runtime/game/state/usr/01/1001.usr" 'Test Player A'
bump_one "$runtime/game/state/usr/02/1002.usr" 'Test Player B'

echo 'Both characters are level 2. Start the server to load them.'
