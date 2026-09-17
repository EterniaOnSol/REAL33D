# HANDOFF

Date/time: 2026-09-16
Agent: Claude
Role: VISUAL REFERENCE PACK
Branch: `main`
Starting commit: `dde7cd6`
Implementation commit: `3e0bea0`
Ending commit: this handoff commit
Worktree: clean after the focused reference pack commit
Remote: `origin` = `https://github.com/EterniaOnSol/REAL33D.git`, `HEAD == origin/main`

## Objective

Complete `VISUAL-REFERENCE-PACK-001`: turn the master visual inventory into a
navigable workstation where the artist can see what to reinterpret, with the
original visual reference, without needing to understand Fusion32, Protocol772,
`objects.srv`, internal ids or the master CSVs. Do not modify ClientCore,
protocol or gameplay. No 3D art. No Unreal.

## Result

`PASS`. 5,284 appearances decoded and catalogued, 10,926 sprites decoded with
zero failures, 5,284 previews rendered with zero failures, 456 of them the
Rookgaard P0 queue. Full counts in
`evidence/visual/VISUAL-REFERENCE-PACK-001.md`.

## The visual source

Found already present and pinned in the project's own authorised workspace by
`CLASSIC-CLIENT-772-001`:

```text
build/classic-client-772/app/Tibia.dat   3C5E857F...9677AEDD
build/classic-client-772/app/Tibia.spr   86ABBF5F...CCE0C64A
```

Nothing was downloaded, no substitute was used, and neither file is committed.

One argument for the version **does not hold**, and the evidence says so:
`reference/login/src/connections.cc` lines 598-600 read `DATSIGNATURE`,
`SPRSIGNATURE` and `PICSIGNATURE` and discard them, so a successful live login
proves nothing about which appearance data the client used. What does support it
is that the client's declared counts line up with the server's own content three
independent ways, listed in the evidence. Provenance of the copy itself remains
`UNKNOWN`, unchanged.

## Format established, not assumed

No `Tibia.dat` specification exists in this project's source truth, so
`visual/tools/tibia772.py` treats the layout as a hypothesis and proves it four
ways: the records consume the file to exactly EOF, the header counts match,
every sprite id falls inside the sprite file, and the client's option bytes
agree with the server's object flags across thousands of items.

That last check is the independent one, since the reader never opens
`objects.srv`. Nine of fifteen correlations are exact; the rest run 97.5 to
99.0 percent with disagreement almost entirely one-directional, the server
carrying flags the client need not draw. A misread layout would give scattered
two-way noise instead.

`patternZ` is present in this format. The first ground reads
`01 01 01 04 04 01 01` followed by sprite ids 136..151: sixteen sprites and
exactly sixteen valid ids. Omitting the field would shift every later read out
of range, which is what makes it detectable rather than assumed.

## Visual families revalidated

The master inventory's 1,351 groups came from name and category, which was
inference. Against real artwork: 4,587 distinct sprite sets across the 5,284
appearances, 310 of them used by more than one appearance covering 835 ids.
Those 310 are **demonstrated** identical pictures.

The docs state plainly that neither figure is a model count. A wall drawn from
four directions is four sprite sets and one mesh; a 4x4 ground pattern is one
sprite set that may want one mesh plus a varying material. `Representation` in
the tracker stays editable because that call belongs to the artist.

Every Fusion32 id keeps its mapping: `appearances.csv` carries the tracker id,
client id, inventory name, category, priority and visual group on every row.

## Unresolved, named rather than hidden

- **100 outfits** the client ships that no monster file, npc file or
  player-selectable range references. Real appearances with real sprites,
  catalogued and previewed, with nothing in the server data pointing at them.
- **Object type 5090**, `a treasure map`, declared server side but the client's
  item ids stop at 5089. Ranges are otherwise contiguous and gapless on both
  sides, so this is one known id, not general ambiguity.
- **36 empty sprite slots** of 10,962.

All 5,284 appearances map to exactly one inventory identity. No association is
ambiguous.

## Repository policy

Neither the client data nor the 99 MB of derived previews is committed.
`/visual/reference_pack/` was added to `.gitignore` because previews are
derivative works of an artifact with `UNKNOWN` provenance. The existing LFS
rules for future original 3D assets are unchanged; no binary was committed.

`visual/tools/setup_reference_pack.sh` is the single setup command. It verifies
both client files against their recorded SHA-256 and refuses to continue on a
mismatch, so a different client version cannot quietly produce a catalogue
describing a different game.

The flow for a fresh clone is: clone, drop the two authorised client files into
`build/classic-client-772/app/`, run that one script, open
`visual/reference_pack/catalogue.html`.

## Tests

`visual/tools/test_tibia772.py`, 24 tests, all passing. Every fixture is built
byte by byte inside the test, so the suite runs without the client data and
nothing proprietary is committed. Covers geometry, the `patternZ` shape, the
exact-size rule, option payloads, the correlation check in both directions, RLE
decoding, PNG output and nearest-neighbour scaling; failure cases include
trailing bytes, every truncation of a file, unknown option bytes, RLE overruns,
a truncated offset table and a short header.

```powershell
wsl.exe -d Ubuntu-26.04 -- python3 /mnt/c/Users/dell/Desktop/fusion32/visual/tools/test_tibia772.py
```

## Checks

- Parser tests: 24/24 `PASS`
- `tests/secret_check.sh`: `PASS`
- `reference/` untouched
- `clientcore/` untouched; no protocol or gameplay change
- No 3D art, no Unreal work
- No proprietary or derived binary committed

## Exact next task

`ROOKGAARD-P0-MOCKUPS-001`, and it is now unblocked in a way it was not before.

The artist can open `visual/reference_pack/p0_rookgaard.html`, see all 456 P0
object types with their original sprites, and start proposing. Work the group
representatives first, the rows whose `Representation` is `MESH`, since each
covers several ids. Record the version in the tracker's `Mockup Version`, move
the row to `REVIEW`, and the project director decides `APPROVED` or `REJECTED`.
There is no fidelity percentage; `visual/docs/ART_DIRECTION.md` is unchanged.

The alternative remains `UNREAL-SLICE-001`: the protocol side has been ready
since `TWO-CLIENT-VERTICAL-SLICE-001`, and
`visual/docs/TECHNICAL_STANDARD.md` holds the provisional scale, pivots and
naming plus the floor-height question it will have to settle.

Do not begin either automatically.

Commands to reproduce this task's results:

```powershell
wsl.exe -d Ubuntu-26.04 -- python3 /mnt/c/Users/dell/Desktop/fusion32/visual/tools/test_tibia772.py
wsl.exe -d Ubuntu-26.04 -- bash /mnt/c/Users/dell/Desktop/fusion32/visual/tools/setup_reference_pack.sh
wsl.exe -d Ubuntu-26.04 -- bash /mnt/c/Users/dell/Desktop/fusion32/tests/secret_check.sh /mnt/c/Users/dell/Desktop/fusion32
```
