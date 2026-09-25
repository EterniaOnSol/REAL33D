"""Retain a replayable Aldric trace without local paths or raw viewport tiles.

Usage: python tests/sanitize_aldric_trace.py raw.jsonl public.jsonl
The raw file remains local and ignored; the public file is reviewed before Git add.
"""

import json
import sys
from pathlib import Path


def main() -> None:
    source, target = (Path(value) for value in sys.argv[1:3])
    if target.exists():
        raise SystemExit(f"refusing to overwrite {target}")
    events = []
    for number, line in enumerate(source.read_text(encoding="utf-8").splitlines(), 1):
        event = json.loads(line)
        if event.get("seq") != number:
            raise SystemExit(f"noncontiguous sequence at line {number}")
        if event.get("event") == "observation":
            observation = event["observation"]
            event["observation"] = {
                "schema": observation["schema"],
                "self": observation["self"],
                "visible": {"creatures": observation["visible"]["creatures"]},
            }
        elif event.get("event") == "session_start":
            event["memory"]["file"] = "<local personal memory file>"
        elif event.get("event") == "session_end":
            event["memory_file"] = "<local personal memory file>"
        elif event.get("event") == "memory_saved":
            event["file"] = "<local personal memory file>"
        events.append(event)
    if not events or events[0].get("event") != "session_start" or events[-1].get("event") != "session_end":
        raise SystemExit("complete session required")
    session_ids = {event.get("session_id") for event in events}
    if len(session_ids) != 1:
        raise SystemExit("one session required")
    target.write_text(
        "".join(json.dumps(event, sort_keys=True, separators=(",", ":")) + "\n" for event in events),
        encoding="utf-8",
    )
    print(f"sanitized {len(events)} events for session {events[0]['session_id']}")


if __name__ == "__main__":
    if len(sys.argv) != 3:
        raise SystemExit(__doc__)
    main()
