#!/usr/bin/env python3
"""Scan produced logs and evidence for this runtime's own credential values.

WHY THIS EXISTS. UNREAL-LOGIN-CHARACTER-SELECT-UI-001 was certified against a
live Fusion32 and failed on one criterion: the client logged the raw account
number, so the runtime account id appeared four times in the retained raw text
-- `REAL33D.log` three times and `login_qa.jsonl` once -- while passwords
appeared zero times. `tests/secret_check.sh` did not and does not catch that: it
bans tracked secret FILES and `KEY=VALUE` assignments, not a bare number inside
a log line. This is the check that would have caught it, and the one that keeps
it caught.

WHAT IT CHECKS. The literal values in the runtime's own `secrets/credentials.env`
-- both account ids and both passwords -- against every text artifact under the
given targets. Account ids are matched on digit boundaries, so the protocol
version 772 in a path does not look like account 772001 and vice versa; a
password is matched as a plain substring, because any occurrence of one is a
leak however it is surrounded.

WHAT IT NEVER DOES. Print a credential. Output is counts, credential KIND names
and file paths only, so the report this writes can itself be packaged as
evidence. That is also why the needles come from the gitignored runtime rather
than from anything tracked.

SELF-TESTED FIRST. A scan that cannot match is indistinguishable from a clean
result, so each needle is matched against a planted copy of itself before the
targets are read, and a needle that fails to self-test is an error rather than a
pass -- the same rule tests/secret_check.sh applies to its PEM pattern.

  tests/scan_credential_values.py --runtime DIR [--out FILE] TARGET [TARGET ...]
  tests/scan_credential_values.py --runtime DIR --redact TARGET [TARGET ...]

`--redact` is how an artifact that ALREADY carries a value is sanitized, and it
is the only supported way: it replaces each occurrence in place with
`[REDACTED_<KEY>]` -- the marker the independent certification of
UNREAL-LOGIN-CHARACTER-SELECT-UI-001 used for its own published copies, so a
sanitized line from either source reads the same -- and then re-scans to prove
the result is clean. The surrounding diagnostic is left exactly as it was, which
is the point: the line still says a login was started and where to, it just no
longer says as whom. Redaction is a content edit to committed evidence and
belongs in a normal commit; nothing here rewrites history.

Exit: 0 clean, 1 at least one occurrence, 2 the scan could not be made
      trustworthy (no credentials file, no needles, or a failed self-test).
"""

from __future__ import annotations

import argparse
import json
import re
import sys
from pathlib import Path

# Machine-written text only. A milestone write-up is prose that may quote the
# very finding this scans for, and the account ids are also plain literals in
# the tracked scripts/server/prepare_wsl.sh that generates the runtime; sweeping
# either would report the document that describes the defect as if it were the
# defect. Named targets are still scanned whatever their suffix, because naming
# a file is the caller saying they meant that file.
TEXT_SUFFIXES = {".log", ".txt", ".json", ".jsonl", ".md", ".csv", ".xml"}

INTERESTING_KEYS = (
    "ACCOUNT_A_ID",
    "ACCOUNT_B_ID",
    "ACCOUNT_A_PASSWORD",
    "ACCOUNT_B_PASSWORD",
)


def read_needles(runtime: Path) -> dict[str, str]:
    """The runtime's own credential values, by key. Never printed."""
    path = runtime / "secrets" / "credentials.env"
    if not path.is_file():
        raise FileNotFoundError(path)
    needles: dict[str, str] = {}
    for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
        if "=" not in line:
            continue
        key, value = line.split("=", 1)
        key, value = key.strip(), value.strip()
        if key in INTERESTING_KEYS and value:
            needles[key] = value
    return needles


def pattern_for(key: str, value: str) -> re.Pattern[str]:
    # An id is a number and must not match inside a longer one: 772 in a path is
    # not account 772001, and account 772001 is not a hit for a run numbered
    # 7720011. A password is matched literally, anywhere.
    if key.endswith("_ID"):
        return re.compile(r"(?<!\d)" + re.escape(value) + r"(?!\d)")
    return re.compile(re.escape(value))


def self_test(needles: dict[str, str]) -> list[str]:
    """Needles whose own pattern cannot find them. Must be empty."""
    broken = []
    for key, value in needles.items():
        planted = f"line before\nvalue {value} here\nline after\n"
        if not pattern_for(key, value).search(planted):
            broken.append(key)
    return broken


def files_under(target: Path):
    if target.is_file():
        yield target
        return
    for path in sorted(target.rglob("*")):
        if path.is_file() and path.suffix.lower() in TEXT_SUFFIXES:
            yield path


def main() -> int:
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument("--runtime", required=True,
                        help="runtime root holding secrets/credentials.env")
    parser.add_argument("--out", help="write the JSON result here as well")
    parser.add_argument("--redact", action="store_true",
                        help="replace occurrences in place with [REDACTED_<KEY>], "
                             "then re-scan to prove the result is clean")
    parser.add_argument("targets", nargs="+",
                        help="files or directories of produced logs/evidence")
    args = parser.parse_args()

    try:
        needles = read_needles(Path(args.runtime))
    except OSError as error:
        print(f"UNTRUSTWORTHY: no readable credentials file ({error.strerror or error})",
              file=sys.stderr)
        return 2
    if not needles:
        print("UNTRUSTWORTHY: the credentials file named none of the expected keys",
              file=sys.stderr)
        return 2

    broken = self_test(needles)
    if broken:
        print("UNTRUSTWORTHY: these patterns cannot match their own value: "
              + ", ".join(sorted(broken)), file=sys.stderr)
        return 2

    patterns = {key: pattern_for(key, value) for key, value in needles.items()}
    redacted: list[dict[str, object]] = []

    if args.redact:
        # Done before the scan below, so the scan that decides the exit status
        # reads the sanitized files and a redaction that silently missed an
        # occurrence still fails.
        for target in args.targets:
            root = Path(target)
            if not root.exists():
                continue
            for path in files_under(root):
                # newline="" on both sides, so line endings survive untranslated.
                # An evidence file whose CRLFs became LFs would show up in a diff
                # as a rewritten artifact rather than a redacted one, and the
                # claim being made is that only the credential changed.
                with path.open("r", encoding="utf-8", errors="replace",
                               newline="") as handle:
                    text = handle.read()
                replaced = text
                here = 0
                for key, pattern in patterns.items():
                    replaced, count = pattern.subn(f"[REDACTED_{key}]", replaced)
                    here += count
                if here:
                    with path.open("w", encoding="utf-8", newline="") as handle:
                        handle.write(replaced)
                    redacted.append({"file": str(path), "occurrences": here})

    hits: list[dict[str, object]] = []
    scanned = 0
    missing = [t for t in args.targets if not Path(t).exists()]

    for target in args.targets:
        root = Path(target)
        if not root.exists():
            continue
        base = root.parent if root.is_file() else root
        for path in files_under(root):
            scanned += 1
            text = path.read_text(encoding="utf-8", errors="replace")
            for key, pattern in patterns.items():
                count = len(pattern.findall(text))
                if count:
                    try:
                        shown = str(path.relative_to(base))
                    except ValueError:
                        shown = str(path)
                    hits.append({"file": shown, "credential_kind": key,
                                 "occurrences": count})

    result = {
        "files_scanned": scanned,
        "credential_kinds_searched": sorted(needles),
        "patterns_self_tested": True,
        "account_id_occurrences":
            sum(int(h["occurrences"]) for h in hits
                if str(h["credential_kind"]).endswith("_ID")),
        "password_occurrences":
            sum(int(h["occurrences"]) for h in hits
                if str(h["credential_kind"]).endswith("_PASSWORD")),
        "hits": hits,
        "redacted": redacted,
        "targets_missing": missing,
    }
    report = json.dumps(result, indent=2, sort_keys=True)
    print(report)
    if args.out:
        Path(args.out).parent.mkdir(parents=True, exist_ok=True)
        Path(args.out).write_text(report + "\n", encoding="utf-8")

    if scanned == 0:
        print("UNTRUSTWORTHY: no text artifact was in scope", file=sys.stderr)
        return 2
    if hits:
        print(f"CREDENTIAL VALUES IN OUTPUT: {len(hits)} file/kind pair(s)",
              file=sys.stderr)
        return 1
    print("CREDENTIAL SCAN: CLEAN "
          f"({scanned} file(s), {len(needles)} value(s), patterns self-tested)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
