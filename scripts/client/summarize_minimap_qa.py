"""Preserve compact observations and request counters without raw runtime logs."""
import argparse
import collections
import json
from pathlib import Path
import re


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("session", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    log = (args.session / "REAL33D.log").read_text(encoding="utf-8", errors="replace")
    observations = []
    for match in re.finditer(r"\[([\d.:-]+)\].*minimap observation: player=(\d+),(\d+),(\d+) known=(\d+) live=(\d+) viewport=18x14", log):
        observations.append(dict(time=match[1], position=list(map(int, match.group(2,3,4))),
                                 known=int(match[5]), live=int(match[6])))
    transitions = []
    for before, after in zip(observations, observations[1:]):
        if before["position"][2] != after["position"][2]:
            transitions.append(dict(before=before, after=after))
    snapshots = []
    for path in sorted(args.session.glob("unreal_slice_evidence*.json")):
        data = json.loads(path.read_text(encoding="utf-8-sig"))
        snapshots.append({key: data.get(key) for key in (
            "written_at", "reason", "seconds_in_session", "local_position", "anchor",
            "minimap", "clientcore", "presentation", "inventory_known", "open_containers")})
    journal = args.session / "movement_journal.jsonl"
    movement = [json.loads(line) for line in journal.read_text(encoding="utf-8-sig").splitlines() if line.strip()] if journal.exists() else []
    accepted = collections.Counter(row["direction"] for row in movement if row["phase"] == "accepted")
    result = dict(schema="real33d.minimap.qa.v1", session=args.session.name,
                  observations=observations, floor_transitions=transitions,
                  accepted_by_direction=dict(accepted), movement=movement, snapshots=snapshots,
                  cache_events=re.findall(r"minimap cache (?:load|saved):[^\r\n]*", log),
                  ui_events=re.findall(r"minimap (?:destination|pan|recenter):?[^\r\n]*", log),
                  navigation_events=[dict(time=match[1], event=match[2]) for match in
                      re.finditer(r"\[([\d.:-]+)\].*LogReal33D: ((?:left-click walk|input \d+: left-click walk)[^\r\n]*)", log)],
                  combat_events=[dict(time=match[1], event=match[2]) for match in
                      re.finditer(r"\[([\d.:-]+)\].*LogReal33D: ((?:combat input|combat state)[^\r\n]*)", log)],
                  limitations="Counters and renderer traces do not substitute for visual/interaction verification.")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(f"SESSION={args.session.name}\nOBSERVATIONS={len(observations)}\nFLOOR_TRANSITIONS={len(transitions)}\nACCEPTED_BY_DIRECTION={dict(accepted)}")


if __name__ == "__main__":
    main()
