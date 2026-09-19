# Unreal Vertical Slice

Task: `UNREAL-SLICE-001`

Status: see the live run section. Everything before it is true regardless of
the run's outcome.

## What this milestone is and is not

It is the first time anything in this project draws. It is not an attempt to
make the world look like Tibia: every mesh on screen is an engine primitive.
What is being tested is the chain, not the picture.

```text
Fusion32 -> Protocol772Core -> semantic events -> WorldState
         -> Unreal bridge -> game thread -> 3D representation
```

Protocol772Core stays the client's authority. Unreal parses nothing, decides
nothing, and stores no world of its own beyond the actors it has been told to
spawn.

## The boundary, and what enforces it

The rule is that Unreal never sees a byte of Protocol 772. Three things hold it
up, and none of them are conventions anyone has to remember:

**One implementation, linked not copied.** `unreal/REAL33D/Source/REAL33D/
REAL33D.Build.cs` links `build/clientcore-windows/protocol772core.lib` and
refuses to build without it. There is no protocol source inside the Unreal
module to drift from `clientcore/`, because there is no protocol source inside
the Unreal module at all.

**One file may include a protocol header.** `Private/Real33DBridge.cpp`, and it
is where the worker thread lives. No public header of this module mentions
`fusion32::protocol772`, so nothing above the bridge can reach the protocol even
by accident. `Real33DCoords.h` mirrors `MapPosition` as three plain integers
rather than including the real one, for exactly this reason.

**The diff lives in ClientCore, not Unreal.** `clientcore/src/worldview.cpp`
turns successive `WorldState`s into semantic events. It is engine-free, it is
covered by `clientcore/tests/worldview_tests.cpp`, and it means the translation
from "the world changed" to "these things changed" is tested by the same
deterministic suites as everything else, instead of living in a Tick function
where nothing can reach it.

## Threading

```text
  worker thread                     game thread
  -------------                     -----------
  socket                            DrainEvents()
  Protocol772Core           SPSC     spawn / update / destroy actors
  WorldState             ---queue--> camera, overlay
  WorldView::Diff                    never touches the socket
  publish FReal33DEvent
```

`FReal33DEvent` is a value. It is copied into the queue and copied out; nothing
crosses the boundary by pointer, so there is nothing the game thread can hold
that the worker might free. Stats are read under a lock and returned by value.
`DrainEvents` asserts `IsInGameThread()`, and so does every method that touches
a component.

Walk input travels the other way through a second SPSC queue as a bare
direction byte.

## Coordinates

From `visual/docs/TECHNICAL_STANDARD.md`: **1 SQM = 100 Unreal Units**. The whole
transform is `Real33D::ToWorld` in `Public/Real33DCoords.h` and nothing else in
the module is allowed to invent one.

| Axis | Server | Unreal | Traced to |
| --- | --- | --- | --- |
| East | `+x` | `+X` | `receiving.cc::ReceiveData` dispatches `CGoDirection` with `(+1,0)` for east |
| South | `+y` | `+Y` | the same dispatch uses `(0,-1)` for north |
| Up | `-z` | `+Z` | Tibia `z=0` is the highest floor, `z=15` the deepest |
| Facing | `enums.hh` `DIRECTION_NORTH=0 … WEST=3` | yaw 270/0/90/180 | `Real33D::ToRotation` |

**Floor height is a presentation choice and is marked as such.** The protocol
fixes the horizontal offset per floor and never states a vertical distance;
`TECHNICAL_STANDARD.md` records it as `UNRESOLVED`. The slice uses 100 uu purely
so multi-floor scenes are legible. It is not derived from the data.

**Why the origin is relative.** Tibia coordinates sit around 32,000, which at
100 uu is 3,200,000 uu. Single-precision floats carry about seven significant
digits, so at that magnitude the representable step is roughly a quarter of a
unit and the jitter is visible. The first anchor of a session fixes an origin
and everything is placed relative to it, which keeps the scene within a few
thousand units of zero. The origin is never re-centred afterwards: doing so
would shift every actor in the scene on every step.

The logical position of a creature is always the exact `WorldState` position.
`ToWorld` converts for drawing and decides nothing.

## Movement, in two directions that are not symmetric

**Remote.** `server event -> ClientCore -> WorldState -> WorldView -> bridge ->
AReal33DCreature::CommitPosition`. The actor lands exactly on the SQM the server
named. Between frames the drawn position eases towards the logical one so a step
reads as a step; the logical position has already arrived.

**Local.** `key -> UReal33DBridge::RequestWalk -> worker -> BuildWalkCommand ->
Fusion32`. That is the entire path. There is deliberately no branch of it that
touches an Actor.

```text
NOT:  Unreal input -> move the actor -> hope the server agrees
BUT:  Unreal input -> ask Fusion32 -> WorldState changes -> the actor follows
```

If Fusion32 refuses the step it answers with `SV_CMD_MESSAGE` and
`SV_CMD_SNAPBACK`, neither of which changes map state, so `WorldState` does not
move, so no event is published, so nothing at all happens on screen. The player
did not move and the screen agrees. This is a structural property of the
wiring, not a correction applied afterwards: there is no code path that could
have displaced the actor in the first place.

The counters distinguish the three things that are easy to conflate:
`steps_requested` is intents handed to Fusion32, `steps_accepted` is
local-player moves that actually came back, `steps_rejected` is snapbacks.

## The asset registry

```text
Fusion32 identity -> visual identity -> Unreal asset
```

`UReal33DAssetRegistry` is the only place that turns an identity into a mesh.
Actors ask it and draw what comes back. No Actor and no gameplay code contains a
thing-id-to-mesh mapping, so when the visual pipeline approves real art the swap
happens in one file: no change to Protocol772Core, no change to the actors.

Every resolution today returns `bIsPlaceholder = true` and an engine primitive.
That is a resolution, not a failure. The kind is deliberately **not** guessed
from the type id: inventing an id range would be precisely the hardcoded mapping
the class exists to prevent. Until the inventory lookup exists, structure
decides — a tile knows its ground is the first thing in its stack because
`map.cc::PlaceObject` sorts `BANK` priority first.

`unreal/REAL33D/Content/` is empty on purpose.

## No authored assets at all

The project carries no `.umap`, no `.uasset`, and no input asset. The scene is
built in `AReal33DGameMode::StartPlay` on top of the empty engine map, and walk
keys are bound in C++ with `BindKey`. Everything that defines a run is text in
the diff, and a clean checkout reproduces it without opening the editor.

## Build and run

Protocol772Core is compiled twice, on purpose. UE 5.8 refuses to build a module
at C++17, and mixing standards across a static-library boundary is not something
to be casual about, so `tests/build_clientcore_windows.cmd` builds the same
sources at C++17 — which is what the suites run against, and what proves the
component is still portable C++17 — and again at C++20, which is the archive
Unreal links. Two compilations of one implementation; never two
implementations. Both are `/W4 /WX /permissive-`.

```bat
rem 1. Protocol772Core, both standards, plus all 8 suites
tests\build_clientcore_windows.cmd

rem 2. the Unreal module
"C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" ^
  REAL33DEditor Win64 Development ^
  -Project="%CD%\unreal\REAL33D\REAL33D.uproject" -WaitMutex

rem 3. the client, uncooked, against the running sanitized runtime
scripts\client\run_unreal_slice.cmd B
```

Step 1 must precede step 2: `REAL33D.Build.cs` throws a `BuildException` with
these instructions if the archive is missing, rather than silently building
something that cannot link.

The server side is unchanged from previous milestones:

```bash
wsl -e bash scripts/server/prepare_wsl.sh /mnt/c/Users/dell/Desktop/fusion32
wsl -e bash scripts/server/start_wsl.sh
```

Unreal runs on Windows and reaches the runtime two ways: the secrets and
`objects.srv` are read through the `\\wsl.localhost` share, and the sockets go
to `127.0.0.1`, which WSL2 forwards into the distribution.

### Exact versions

| Component | Version |
| --- | --- |
| Unreal Engine | 5.8, `C:\Program Files\Epic Games\UE_5.8` |
| Target | `REAL33DEditor`, Win64, Development, run with `-game` |
| Toolchain | Visual Studio 14.50.35738, Windows SDK 10.0.26100.0 |
| Module standard | C++20 (`CppStandardVersion.Cpp20`) |
| Protocol772Core | C++17 sources, archived at C++20 for the link |
| OpenSSL | the engine's own 1.1.1t, so one libcrypto is in the process |

Credentials and the RSA modulus are read at runtime from
`<runtime>/secrets/`. Nothing is baked into the binary and nothing is committed.

### Controls

| Gesture | Effect |
| --- | --- |
| `W` `A` `S` `D`, arrow keys | Request a step. Inert while the chat box has focus. |
| `Enter` | Put the caret in the chat box, and send the line when it is already there. |
| `Escape` | Abandon the line and give movement back. |
| Right mouse button held, mouse moved | Orbit the camera around the player. |
| Mouse wheel | Move the camera in and out. |
| `F9` | Write a labelled evidence snapshot. |

All of it is bound in code with `BindKey` and `BindAxisKey`; the project ships no
binary input assets. The orbit's limits live in `AReal33DWorld`, not in the
controller: pitch is clamped between -85 and -5 degrees and distance between 400
and 3000 units, and the defaults reproduce the fixed view the camera had before
the orbit existed.

## Live run

<!-- LIVE RUN RESULTS -->

## Out of scope, and absent

No inventory, containers, combat UI, spells, runes, final effects, equipment
visuals, full UI, minimap, audio, final art, mobile support, offline full-map
conversion, map editor or server-side change.

Chat is no longer in this list. `SV_CMD_TALK` is decoded from the three
`SendTalk` overloads, outgoing talk carries a semantic mode mapped to the wire
only inside the bridge, and a Slate chat area renders the transcript; see
`docs/UNREAL_CHAT.md` and `evidence/clientcore/unreal-slice/chat_area_live.md`.
The earlier instruction to stay out of the in-game chat no longer applies.
