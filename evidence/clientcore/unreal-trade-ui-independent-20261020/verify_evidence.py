"""Read-only independent evidence audit. Run with Python 3 from any cwd."""
import json
import re
from pathlib import Path

root = Path(__file__).resolve().parent
trade = (root / "live/trade/REAL33D.log").read_text(encoding="utf-8")
resume = (root / "live/resume/REAL33D.log").read_text(encoding="utf-8")
required_trade = [
    "Ctrl right click real item slot",
    "context menu sends neither Use nor Trade",
    "actual Trade with menu arms target",
    "actual player button completes Trade target",
    "request clears targeting but waits for server offer",
    "server own offer; partner prose is not invented offer",
    "both reciprocal server offers",
    "own inspect received matching server description",
    "partner inspect received matching server description",
    "one accept waits; received ownership unchanged",
    "AUTHORITATIVE TRANSFER: both owners changed in received inventory/container deltas",
    "B Reject actual button clears locally",
    "A Reject actual peer button",
    "only partner receives CLOSE_TRADE on reject",
    "registry hide reopen preserves received trade",
]
required_resume = [
    "resume confirms previously transferred ownership from fresh server snapshots",
    "reciprocal trade restores original ownership",
    "UseWith still routes to Use; no trade command",
    "channel real echo isolated and painted",
    "Local Chat real echo isolated and painted",
    "text editor renders exact server body",
    "text editor Close",
    "final original ownership and idle trade",
]
for text, checks in ((trade, required_trade), (resume, required_resume)):
    for check in checks:
        assert f"CERT_ASSERT PASS {check}" in text, check
assert "TRADE_CERT_RESULT=PASS step=21 round=3" in resume
assert "CERT_ASSERT FAILED" not in resume
# Preserve the interrupted run honestly rather than silently treating it as PASS.
assert "TRADE_CERT_RESULT=FAILED step=3 round=3" in trade

counter_pattern = re.compile(
    r"CERT_COUNTERS ([AB]) frames=(\d+) commands=(\d+) "
    r"residual=(\d+) unsupported=(\d+) anomalies=(\d+)"
)
counter_rows = []
for label, text in (("trade", trade), ("resume", resume)):
    rows = counter_pattern.findall(text)
    assert rows, label
    for who, frames, commands, residual, unsupported, anomalies in rows:
        assert (int(residual), int(unsupported), int(anomalies)) == (0, 0, 0)
    for who in ("A", "B"):
        last = [r for r in rows if r[0] == who][-1]
        counter_rows.append(dict(session=label, account=who, frames=int(last[1]),
                                 commands=int(last[2]), residual=0,
                                 unsupported=0, anomalies=0))

snapshots = []
for path in sorted((root / "live").rglob("*.json")):
    doc = json.loads(path.read_text(encoding="utf-8-sig"))
    stats = doc["clientcore"]
    for field in ("residual_bytes", "unsupported_opcodes", "protocol_anomalies"):
        assert stats[field] == 0, (path.name, field)
    assert doc["last_diagnostic"] == "none", path.name
    snapshots.append(str(path.relative_to(root)))


def body(path):
    doc = json.loads((root / path).read_text(encoding="utf-8-sig"))
    objects = [(f"inventory:{i['slot']}", i["type_id"]) for i in doc["inventory"]]
    for container in doc["containers"]:
        objects.extend((f"container:{container['number']}:{i['index']}", i["type_id"])
                       for i in container["objects"])
    return objects


b_baseline = body("live/trade/unreal_slice_evidence_CertBaseline0.json")
b_transfer = body("live/trade/unreal_slice_evidence_CertTransfer0.json")
b_final = body("live/resume/unreal_slice_evidence_CertFinal3.json")
assert ("inventory:5", 3294) in b_baseline
assert not any(t == 3561 for _, t in b_baseline)
assert ("container:0:0", 3561) in b_transfer
assert not any(t == 3294 for _, t in b_transfer)
assert b_final == b_baseline


def peer_after_assert(text, marker):
    tail = text.split(f"CERT_ASSERT PASS {marker}", 1)[1]
    tail = tail.split("CERT_STAGE_COMPLETE", 1)[0]
    return re.findall(r"CERT_ITEM A inventory=(\d+) type=(\d+)", tail)


a_transfer = peer_after_assert(
    trade, "AUTHORITATIVE TRANSFER: both owners changed in received inventory/container deltas"
)
a_final = peer_after_assert(resume, "final original ownership and idle trade")
assert ("10", "3294") in a_transfer
assert not any(t == "3561" for _, t in a_transfer)
assert ("5", "3561") in a_final
assert not any(t == "3294" for _, t in a_final)
print(json.dumps({
    "audit": "PASS",
    "named_live_assertions": len(required_trade) + len(required_resume),
    "snapshot_count": len(snapshots),
    "final_counters": counter_rows,
    "b_original_inventory_and_backpack_restored": True,
    "a_received_transfer_and_return_confirmed": True,
    "interrupted_trade_run_retained_as_failed": True,
    "resume_run_result": "PASS",
}, indent=2))
