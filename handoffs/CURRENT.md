# HANDOFF

Date/time: 2026-09-18
Agent: Claude
Role: MINIMAL PLAYABLE CHAT SURFACE
Branch: `main`
Milestone: `UNREAL-CHAT-AREA-001`
Status: **IN_PROGRESS** — ClientCore foundation landed, Unreal UI not started.

Previous milestone `UNREAL-CHAT-OUTGOING-001` is PASS and published; its record
is `evidence/clientcore/unreal-slice/yell_live.md` and `docs/UNREAL_CHAT.md`.

## What this commit contains

`ChatLog` in ClientCore: a bounded, ordered transcript of what the player should
be able to read, with deterministic tests. Nothing in the Unreal module changed.

It lives in ClientCore, not in Unreal, because the rules worth getting right
here — ordering, bounding, eviction, formatting — are testable without a
renderer. Whether the operator can actually *read* the result is a separate
question only a live run answers, and this commit does not claim it.

    CHAT_HISTORY_CAPACITY = 60   REAL33D_UI_BEHAVIOUR, not 7.72 parity

Nothing in Fusion32 states how many lines a client retains, because the server
keeps no transcript at all.

Deliberately NOT in `WorldState`. WorldState is what the server says the world
is; a transcript is what this client chose to keep on screen.

## Audit findings that shape the remaining work

1. **The module has no Slate/UMG dependency and no widgets exist.** There is no
   player-facing UI layer at all today. Plan: **pure Slate**, no `.uasset`,
   matching the project's existing choice to bind input in code rather than ship
   binary input assets. `Slate` + `SlateCore` added to `REAL33D.Build.cs`.

2. **Server messages are currently `Diagnostic` events** carrying the prose
   `"server message [LoginMessage]: ..."`. The milestone brief requires player
   messages and protocol diagnostics to be different things. A dedicated
   `EReal33DEventKind::ServerMessage` is needed; the chat area must never
   receive protocol diagnostics.

3. **Outgoing talk mode travels as a typed `"#y "` prefix** parsed inside the
   bridge. The brief forbids the UI mapping to protocol values, so a semantic
   `EReal33DTalkMode { Say, Whisper, Yell }` is needed, mapped to the wire only
   inside `Real33DBridge.cpp` — the one file permitted to include a protocol
   header.

## Next steps, in order

1. `REAL33D.Build.cs`: add `Slate`, `SlateCore`.
2. `EReal33DTalkMode` + `UReal33DBridge::RequestTalk(Mode, Text)` replacing
   `RequestSay`; delete the `#y ` prefix parsing in `DrainSays`.
   **This refactor was started and reverted in this session because it was
   incomplete and would not compile.** Do it in one pass: header enum, bridge
   mapping, `FSay` gains a mode field, `Real33DPlayerController` updated.
3. `EReal33DEventKind::ServerMessage`, published where `ProcessPayload`
   currently emits a `Diagnostic` for `ServerUpdateKind::Message`.
4. `SReal33DChatPanel`: history list, Say/Whisper/Yell selector, editable text
   box, Send button. Added via `GEngine->GameViewport->AddViewportWidgetContent`.
5. Focus gating: movement suppressed while the text box holds focus. Gate it on
   an explicit flag rather than relying on Slate focus semantics, so it is
   deterministic and provable.
6. Remove the fallback's `AddOnScreenDebugMessage` rendering; the chat area
   replaces it. Keep the counters overlay separate.

## The defect this milestone exists to close

    DISTANT_SPEECH_READABLE = NO

Proven live: `talk [Yell] at 32098,32204,7, Test Player A: "YOU SURE?" speaker
NoMatch creature 0`. A was nine fields away, the viewport reaches six, so there
was no actor to draw text above. Resolution and routing are correct; the
fallback is drawn into the diagnostics overlay, where the operator cannot read
it. Do not fix this by inventing an actor or widening the viewport.

## Environment notes

- Test Player B is now level 2, so `TALK_YELL` is no longer refused.
  `YELL_MIN_LEVEL = 2` from `receiving.cc::CTalk`.
- **The server caches players in memory and flushes on shutdown.** Editing a
  `.usr` file while the server runs is silently discarded and then overwritten.
  Any save-data change must be made with the server stopped.
- Backup of B's pre-bump file:
  `game/state/usr/02/1002.usr.bak-before-level-bump` inside the WSL runtime.

## Qualifications that must not be quietly dropped

    SPEECH_LIFETIME_PARITY = NOT_PROVEN
    CONSECUTIVE_WORLD_SPEECH_PARITY = NOT_PROVEN
    EXACT_SPEECH_COLOR_PARITY = NOT_PROVEN
    TALK_MODE_PERSISTENCE = REAL33D_UI_BEHAVIOUR, not proven parity
