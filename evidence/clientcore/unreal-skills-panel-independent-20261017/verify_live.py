"""Replay the retained read-only observer and final protocol snapshots."""
import json
import re
from pathlib import Path

root = Path(__file__).resolve().parent
live = root / "live"
log = (live / "REAL33D.log").read_text(encoding="utf-8-sig")
names = ["Level", "Experience", "Hit Points", "Mana", "Soul Points",
         "Capacity", "Magic Level", "Fist Fighting", "Club Fighting",
         "Sword Fighting", "Axe Fighting", "Distance Fighting", "Shielding", "Fishing"]
pattern = re.compile(r"SkillsCert LIVE known=(\d)/(\d) vitals=([\d,]+) skills=([\d:,]+) rendered=(.*?) painted_percent=([\d.,]+)")
records = []
for match in pattern.finditer(log):
    vk, sk = map(int, match.group(1, 2))
    v = [int(x) for x in match[3].split(",") if x]
    s = [tuple(map(int, x.split(":"))) for x in match[4].split(",") if x]
    rendered = dict(x.split("=") for x in match[5].split(";") if x)
    paint = [float(x) for x in match[6].split(",") if x]
    assert list(rendered) == names and len(s) == 7 and len(paint) == 9
    expected = [v[0], v[2], v[3], v[4], v[5], v[6], v[7]] + [x[0] for x in s]
    expected = [str(x) if (vk if i < 7 else sk) else "--" for i, x in enumerate(expected)]
    expected_paint = ([v[1], v[8]] if vk else [0, 0]) + ([x[1] for x in s] if sk else [0] * 7)
    matches = list(rendered.values()) == expected and all(abs(a-b) < .02 for a,b in zip(paint, expected_paint))
    records.append((vk, sk, v, s, matches, match.start()))

assert records and any(not r[0] and not r[1] and r[4] for r in records)
known = [r for r in records if r[0] and r[1]]
assert known and all(r[2][7] == 0 for r in known)
# OnEndFrame can see an event before the next HUD refresh. Every received
# skills vector must nevertheless have a real matching rendered observation.
received = {tuple(r[3]) for r in known}
rendered_states = {tuple(r[3]) for r in known if r[4]}
assert received <= rendered_states
assert known[0][3][2] == (11, 67) and known[-1][3][2] == (11, 83)
assert known[-1][4] and known[0][2][2] == 471 and known[-1][2][2] == 486
attacks = list(re.finditer(r"combat input \d+ from world right-click: attack creature", log))
assert len(attacks) == 3
first_progress = next(r for r in known if r[3][2][1] > 67)
assert attacks[0].start() < first_progress[5]
assert "LogExit: Exiting." in log
snapshots = sorted(p for p in live.glob("unreal_slice_evidence*.json") if not p.name.endswith(".geometry.json"))
assert snapshots
for path in snapshots:
    data = json.loads(path.read_text(encoding="utf-8-sig"))
    cc = data["clientcore"]
    assert all(cc[k] == 0 for k in ("residual_bytes", "unsupported_opcodes", "protocol_anomalies")), path
final = json.loads((live / "unreal_slice_evidence.json").read_text(encoding="utf-8-sig"))
assert final["reason"] == "EndPlay" and final["clientcore"]["attacks_requested"] == 3
summary = dict(status="PASS", observer_records=len(records),
               matching_records=sum(r[4] for r in records),
               pre_refresh_transitions=sum(not r[4] for r in records),
               received_skill_vectors=len(received), all_vectors_rendered=True,
               sword_before="11/67%", sword_after="11/83%", experience=[471,486],
               ordinary_world_attacks=len(attacks), snapshots=len(snapshots),
               final_frames=final["clientcore"]["frames"],
               final_commands=final["clientcore"]["commands"],
               residual_bytes=0, unsupported_opcodes=0, protocol_anomalies=0)
print(json.dumps(summary, indent=2))
