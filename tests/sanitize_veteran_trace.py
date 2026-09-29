"""Sanitize one complete VETERAN-PLAY-002 session without losing progression proof.

Usage: python tests/sanitize_veteran_trace.py raw.jsonl public.jsonl
Raw traces stay ignored; the destination must not already exist.
"""
import json
import sys
from pathlib import Path


def main(source, target):
    if target.exists():
        raise SystemExit(f"refusing to overwrite {target}")
    events = []
    for number, line in enumerate(source.read_text(encoding="utf-8").splitlines(), 1):
        event = json.loads(line)
        if event.get("seq") != number:
            raise SystemExit(f"noncontiguous sequence at line {number}")
        kind = event.get("event")
        if kind == "observation":
            observation = event["observation"]
            event["observation"] = {
                "schema": observation["schema"],
                "self": observation["self"],
                "visible": {"creatures": observation["visible"]["creatures"]},
                "owned": observation.get("owned", {}),
                "shop": observation.get("shop", {}),
            }
        elif kind == "session_start":
            if event.get("memory"):
                event["memory"]["file"] = "<local personal memory file>"
        elif kind == "session_end":
            event["memory_file"] = "<local personal memory file>"
        elif kind == "memory_saved":
            event["file"] = "<local personal memory file>"
        events.append(event)
    if not events or events[0].get("event") != "session_start" \
       or events[-1].get("event") != "session_end":
        raise SystemExit("complete session required")
    if len({event.get("session_id") for event in events}) != 1:
        raise SystemExit("one session required")
    target.parent.mkdir(parents=True, exist_ok=True)
    target.write_text("".join(json.dumps(event, sort_keys=True,
        separators=(",", ":")) + "\n" for event in events), encoding="utf-8")
    print(f"sanitized {len(events)} events for session {events[0]['session_id']}")


if __name__ == "__main__":
    if len(sys.argv) != 3:
        raise SystemExit(__doc__)
    main(Path(sys.argv[1]), Path(sys.argv[2]))
