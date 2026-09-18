# UNREAL-CHAT-PRESENTATION-001

Make speech decoded by `CHAT-772-001` actually visible in the Unreal client.

Design, ownership findings and the known cosmetic limitation are in
`docs/UNREAL_CHAT.md`. This file records what was run and what was seen.

## Result

`QUALIFIED PASS`.

Visible incoming **Say** is proven live and confirmed by the operator. Visible
**Whisper** is implemented on the identical code path and was decoded live, but
was **not separately confirmed visible by the operator in this milestone**, so
it is not claimed as such. Lifetime and consecutive-message parity are
`NOT_PROVEN` by design, not by omission.

## The two claims, kept apart

| Claim | Milestone |
| --- | --- |
| The talk packet decodes correctly | `CHAT-772-001` |
| The operator can see the message in Unreal | this one |

`CHAT-772-001` is not retroactively credited with visible chat.

## Ownership, established from source before implementing

**Lifetime is not server-owned and not protocol-owned.** Proven as a negative:
no `SendTalk` overload carries a duration
(`reference/game/src/sending.cc:1328`, `:1365`, `:1402`); the complete server
command list (`reference/game/src/connections.hh:95-139`) contains **no
talk-removal command**; and `StatementID` comes from `LogCommunication` /
`LogListener` (`reference/game/src/operate.cc:2315`), making it a moderation log
id rather than a lifetime handle.

    SPEECH_LIFETIME_OWNER = CLIENT_PRESENTATION

**The client rule itself is not provable here.** `reference/` contains only
server sources; the classic client exists in this project as the binary
`build/classic-client-772/app/Tibia.exe` with no source.

    SPEECH_LIFETIME_PARITY = NOT_PROVEN
    CONSECUTIVE_MESSAGE_PARITY = NOT_PROVEN

A placeholder of 6 s is used, overridable with `-real33d-speech-seconds=`, and
the client logs `speech display lifetime 6.0s (NOT a proven 7.72 value)` on
startup so the value cannot be mistaken for parity. A newer message replaces a
creature's current one: the smallest safe behaviour, not a stacking rule
invented and dressed as 7.72.

**Say versus Whisper visibility is the server's.** Fusion32 chooses the
spectators in `reference/game/src/operate.cc` before anything is sent, so no
range or filtering rule is implemented in Unreal.

## Path

```text
Fusion32 -> SV_CMD_TALK -> DecodeServerUpdate -> TalkUpdate
  -> ResolveTalkSpeaker(WorldState)   [worker thread]
  -> FReal33DEvent --SPSC queue-->    [game thread]
  -> AReal33DWorld::PresentSpeech
       -> AReal33DCreature::ShowSpeech    when a speaker resolved
       -> on-screen fallback line         otherwise
```

The speaker is resolved on the worker thread where `WorldState` lives, so what
crosses the boundary is a creature id the game thread already keys actors by.
Unreal never sees the name, the coordinates or the matching rule, and no opcode
or protocol constant entered the Unreal module.

## Speaker resolution

`ResolveTalkSpeaker` requires **exactly one** visible creature matching the talk
position, and the name too when the talk carries one. Visible means standing on
a stored tile: the known-creature mirror retains creatures that scrolled out of
view and those have no actor to speak above.

An empty sender is not a failure — `reference/game/src/moveuse.cc:941` passes
`""` for the ANIMAL modes — so position alone decides there.

Outcomes are `Resolved`, `NoMatch`, `Ambiguous`, `NotPositional`. Only
`Resolved` puts text above a creature; everything else goes to the fallback,
because speech above the wrong creature is worse than speech merely not in the
world.

## Live result

`Tibia.exe` 7.72 (Player A, operator) + Fusion32 + Unreal (Player B).

`chat_presentation_resolved.log` — every line resolved to the correct creature:

```text
talk [Say] at 32097,32206,7, Test Player A: "se ve hermosoooooo"
    speaker Resolved creature 1001
talk [Say] at 32097,32206,7, Test Player A: "hola hola holaaaa probandoooo..."
    speaker Resolved creature 1001
```

`speaker Resolved creature 1001` is Player A, matched on both position and
name, with creature 1001 being A's id from an entirely separate decoder.

**Operator observation, HUMAN:** asked whether A's speech was visible in Unreal,
the operator answered *"si veo lo que habla el player A"*, and after the
sizing fix, *"se ve hermosoooooo"*. That is the acceptance criterion this
milestone exists for, and it is human evidence, not machine evidence. No
automated visual assertion exists and none is claimed.

### Movement after chat

`chat_presentation_movement_after_chat.log`: with chat presentation active, the
operator's walk input continued to reach Fusion32 and be answered — refusals
logged against their own input ids while walking into a wall. Chat presentation
did not break the input path, the bridge or the game-thread presentation.

## What is not proven here

- **Visible Whisper** was not separately confirmed by the operator in this
  milestone. It decodes live (`CHAT-772-001`) and takes the identical
  presentation path as Say, so it is expected to work, but expectation is not
  observation.
- **Exact speech lifetime and consecutive-message behaviour**, as above.
- **Exact speech colour.** The operator asked for `#ffff00`; the code sets that
  and the screen shows something warmer. Measured, not assumed: gold
  `(255,190,30)` arrived at roughly `(180,171,138)`. Two causes, both in the
  renderer: `UTextRenderComponent` defaults to a **lit** material, so the colour
  is modulated by the key light and the sky light's blue ambient, and the filmic
  tone curve compressed saturation further. The tone curve is now disabled.
  `/Engine/EngineMaterials/UnlitText` gives the exact colour but ignores the
  font alpha and renders every glyph as a filled block — tried live and rejected
  on sight. Readable text in an approximate colour was chosen over an exact
  colour nobody can read.

      SPEECH_COLOUR_EXACT = NOT_ACHIEVED (readable; hue approximate)

## Outgoing chat: implemented and live

`OUTGOING_UNREAL_CHAT = IMPLEMENTED_AND_LIVE_VERIFIED`.

Derived from `reference/game/src/receiving.cc::CTalk`; packet layout and the
client/server mode asymmetry are in `docs/UNREAL_CHAT.md`.

The full round trip, from `chat_outgoing_live.log`:

```text
chat line opened; walk keys are inert
say 12: "hola" handed to Fusion32
talk [Say] at 32096,32206,7, Test Player B: "hola"  speaker Resolved creature 1002
```

A key press became a semantic ClientCore action, reached Fusion32, and came back
67 ms later as an authoritative `SV_CMD_TALK` that resolved to creature 1002,
Player B himself. Nothing was drawn optimistically: the text on screen is the
server's broadcast, not a local echo.

**Operator observation, HUMAN:** *"funciona el chat... lo veo en clasic"* — the
message typed in Unreal was visible to Player A in the original `Tibia.exe`.

## Yell, and why nothing appeared

Yell was finally testable once Player A reached level 2, and the operator
yelled from underground. **Nothing appeared in Unreal, and that is correct.**

`reference/game/src/operate.cc` picks the spectators, with the Fusion32 authors'
own comment on the rule:

```cpp
}else if(Mode == TALK_YELL || Mode == TALK_ANIMAL_LOUD){
    if(DistanceX > 30 || DistanceY > 30) continue;
    // TODO(fusion): This seems to be correct. Underground yells
    // aren't multi floor.
    if(DistanceZ > 0 && (Spectator->posz > 7 || Creature->posz > 7)) continue;
}
```

A was underground and B on floor 7, so the floors differed and one party was
below ground: Fusion32 never sent the command. No talk packet reached the client
at all, which the log confirms. The client drew nothing because there was
nothing to draw.

This is evidence of fidelity rather than a gap: a client that displayed the yell
would be showing something the server deliberately withheld.

The same block holds another server-side decision worth recording: a whisper
beyond one field is delivered to the spectator as the literal text `"pspsps"`
rather than the real message. What a player is allowed to hear is Fusion32's
call, not the client's.

## Tests

Discovered and executed by `tests/build_clientcore_windows.cmd`.

| Suite | Result |
| --- | --- |
| transport | 22/22 |
| crypto | 25/25 |
| login, gamelogin, initial_world, movement, player_state, worldview | `PASS` |
| Windows MSVC `/W4 /WX /permissive-`, C++17 and C++20 | `WINDOWS CLIENTCORE: PASS` |
| WSL GCC + ASan/UBSan via CTest | 8/8 |

Five new deterministic cases cover the resolution policy, including
`TestTalkSpeakerNeverPicksTheWrongCreature`, which asserts that a right name at
the wrong position and a right position with the wrong name both resolve to
nothing rather than to a guess.

## Cleanup

Speech above a creature is a component of that creature's actor, so a creature
leaving the viewport takes its speech with it; there is no registry that can
outlive an actor. `ClearWorld`, which already runs on disconnect and before
reconnect, also empties the fallback list. Nothing is stored in `WorldState`.
