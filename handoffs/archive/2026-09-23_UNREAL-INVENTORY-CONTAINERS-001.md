# HANDOFF ARCHIVE

Date/time: 2026-09-23, America/Guatemala
Task: `UNREAL-INVENTORY-CONTAINERS-001`
Starting commit: `723b557`
Result: `CERTIFIED_PASS`

The live certification opened a bag, matched its contents object-for-object to
the server save, moved an item and observed live updates, closed it through the
7.72 use toggle, opened a nested bag in its own window, completed the real
flour-on-water use-with rule, and retained correct inventory/equipment with
zero residual bytes, unsupported opcodes or protocol anomalies.

One live bug was fixed: extracted item pictures were indexed one sprite late.
`tests/verify_item_sprite_extraction.py` now compares 4,990 items and 8,163
sprites against the independently validated 7.72 reader. Container windows
also render their full capacity and the panel was enlarged at operator request.

Full reproducible evidence and the original substantive handoff detail are
retained in `evidence/clientcore/UNREAL-INVENTORY-CONTAINERS-001.md`, its eight
step snapshots, and the certified session log. Remaining unverified commands
are `CL_CMD_CLOSE_CONTAINER`, `CL_CMD_UP_CONTAINER`, use-on-creature and a map
field target for use-two-objects. V08, WideWorld, REAL33D2D and server protocol
were not changed.
