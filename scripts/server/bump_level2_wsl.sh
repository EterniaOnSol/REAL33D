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
# character holding GAMEMASTER_OUTFIT is raised on its first login. The level
# values written here are copied from exactly those four lines:
#
#     Act = 2    Exp = 100    LastLevel = 100    NextLevel = 200
#
# LastLevel is deliberately absent from the file. `TSkill::Load` at
# `crskill.cc:106` recomputes it as `GetExpForLevel(Act)`, which is 100 for
# level 2, so writing Act, Exp and NextLevel is sufficient and consistent.
#
# BUT SKILL_LEVEL ALONE IS NOT ENOUGH, and that is a real defect in the server
# routine this script used to copy verbatim. A genuine level change goes through
# `TSkillLevel::Jump`, which calls `TSkillAdd::Advance(Range)` on four dependent
# skills (`crskill.cc:358-361`):
#
#     SKILL_HITPOINTS  SKILL_MANA  SKILL_GO_STRENGTH  SKILL_CARRY_STRENGTH
#
# `crplayer.cc:162-168` assigns the SKILL_LEVEL fields directly and never calls
# Jump, so the character ends up at level 2 carrying level-1 stats. That stays
# invisible until the character loses the level again: `TSkillLevel::Decrease`
# does call Jump, with Range = -1, and `TSkillAdd::Advance` has no floor at zero
# (`Max = Max + Range * AddLevel`), so it subtracts an increment that was never
# granted. Observed on Test Player B after a death:
#
#     HP 150 -> 145    Mana 0 -> -5    Speed 70 -> 69    Cap 400 -> 390
#
# Max mana then reaches the wire as `SendWord(Connection, (uint16)MaxManaPoints)`
# in `sending.cc::SendPlayerData`, and (uint16)(-5) is 65531 -- which is what the
# clients display, correctly, because that is the value the server sent.
#
# So this script applies Advance(+1) itself, reading each skill's own AddLevel
# from the file rather than hardcoding it. The result is a consistent character:
# level 2 with level-2 stats, which for a vocationless Rookgaard character is
# HP 155, Mana 5, Speed 71, Cap 410.
#
# The file format is the 15-field Skill line written by `crplayer.cc:2492-2522`:
#
#     Skill = (SkillNr,Actual,Maximum,Minimum,DeltaAct,MagicDeltaAct,Cycle,
#              MaxCycle,Count,MaxCount,AddLevel,Experience,FactorPercent,
#              NextLevel,Delta)
#
# SkillNr 0 is SKILL_LEVEL: fields 2, 12 and 14 are set. SkillNrs 2, 3, 4 and 5
# are the dependent skills: fields 2 and 3 each gain field 11 (AddLevel), which
# is precisely what `TSkillAdd::Advance(1)` does to Act and Max.
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

  # Applying Advance(+1) twice would silently inflate the dependent skills, so
  # refuse a character that is not at level 1.
  local level
  level="$(printf '%s' "$before" | sed 's/^Skill = (0,\([^,]*\).*/\1/')"
  if [[ "$level" != 1 ]]; then
    echo "$name is already at level $level; refusing to bump again." >&2
    echo "  $before" >&2
    return 1
  fi

  cp -p "$file" "$file.bak-before-level-bump"

  awk '
    # Strip "Skill = (" and the trailing ")" into f[1..15]; caller rebuilds.
    function parse(line,   n) {
      sub(/^Skill = \(/, "", line)
      sub(/\)$/, "", line)
      n = split(line, f, ",")
      if (n != 15) { print "unexpected field count: " n > "/dev/stderr"; exit 1 }
      return n
    }
    function emit(n,   i, out) {
      out = f[1]
      for (i = 2; i <= n; i++) out = out "," f[i]
      print "Skill = (" out ")"
    }

    /^Skill = \(0,/ {
      n = parse($0)
      f[2]  = 2     # Actual    -> level 2
      f[12] = 100   # Experience-> GetExpForLevel(2)
      f[14] = 200   # NextLevel -> GetExpForLevel(3)
      emit(n)
      next
    }

    # SKILL_HITPOINTS, SKILL_MANA, SKILL_GO_STRENGTH, SKILL_CARRY_STRENGTH --
    # the four skills TSkillLevel::Jump advances. TSkillAdd::Advance(1) adds
    # AddLevel to both Act and Max, clamping Act to Max.
    /^Skill = \([2345],/ {
      n = parse($0)
      f[2] = f[2] + f[11]
      f[3] = f[3] + f[11]
      if (f[2] > f[3]) f[2] = f[3]
      emit(n)
      next
    }

    { print }
  ' "$file" >"$file.tmp"

  mv "$file.tmp" "$file"
  chmod 600 "$file"

  printf '%s\n  before: %s\n  after:  %s\n' \
    "$name" "$before" "$(grep -m1 '^Skill = (0,' "$file")"
  local nr
  for nr in 2 3 4 5; do
    printf '    skill %s: %s\n' "$nr" "$(grep -m1 "^Skill = ($nr," "$file")"
  done
}

bump_one "$runtime/game/state/usr/01/1001.usr" 'Test Player A'
bump_one "$runtime/game/state/usr/02/1002.usr" 'Test Player B'

echo 'Both characters are level 2. Start the server to load them.'
