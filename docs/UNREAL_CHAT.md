# Visible chat in Unreal

Task: `UNREAL-CHAT-PRESENTATION-001`.

`CHAT-772-001` decoded `SV_CMD_TALK` into a semantic `TalkUpdate` and stopped
there, deliberately. Nothing was drawn, so when Player A spoke from the original
`Tibia.exe` the operator saw nothing in Unreal. This milestone makes speech
visible without moving responsibility between layers.

The two claims are different and are kept apart:

| Claim | Milestone |
| --- | --- |
| The talk packet is decoded correctly | `CHAT-772-001` |
| The message is visible to the operator in Unreal | this one |

## Ownership, established before implementing

The question for each behaviour is *who historically owns it*, not what Unreal
needs.

### Lifetime and disappearance: NOT the server, NOT the protocol

Proven from source, as a negative:

- None of the three `SendTalk` overloads
  (`reference/game/src/sending.cc:1328`, `:1365`, `:1402`) carries a duration.
  Their full parameter lists are `StatementID, Sender, Mode, Text, Data`,
  `StatementID, Sender, Mode, Channel, Text` and
  `StatementID, Sender, Mode, x, y, z, Text`.
- The server command list in `reference/game/src/connections.hh:95-139`
  contains **no talk-removal command**. 170 is the only world talk command;
  171-179 are channel and request-queue management, none of which retracts
  speech already sent.
- `StatementID` is not a lifetime handle. It comes from
  `LogCommunication(CreatureID, Mode, Channel, Text)` in
  `reference/game/src/operate.cc:2315` and is consumed by `LogListener`: it is
  a moderation log id.

So Fusion32 says a thing was said and never mentions it again. Lifetime cannot
be server-owned or protocol-owned.

    SPEECH_LIFETIME_OWNER = CLIENT_PRESENTATION

### But the exact client rule is not provable here

`reference/` contains `game`, `login`, `querymanager` and `ipchanger` — all
server side. The classic client exists in this project only as the binary at
`build/classic-client-772/app/Tibia.exe`. There is no client source to read.

So while ownership is proven, the rule itself is not:

    SPEECH_LIFETIME_PARITY = NOT_PROVEN

Specifically unproven: the base duration, whether it scales with message
length, whether it differs by talk mode, and whether a new message refreshes or
replaces an existing one.

The implementation uses a documented placeholder, overridable at runtime with
`-real33d-speech-seconds=`, and logs on startup that the value is not a proven
7.72 value. It is not represented as parity anywhere.

Runtime observation of `Tibia.exe` could narrow this to an approximation, and
that would be evidence of class `INFERRED`, not source truth. It has not been
promoted to parity here.

### Consecutive messages: same situation

Fusion32 emits one independent talk command per utterance and never refers back
to an earlier one, so the server has no opinion on what happens when two
arrive together. That is client presentation too, and equally unprovable from
this project's material.

    CONSECUTIVE_MESSAGE_PARITY = NOT_PROVEN

The smallest safe behaviour is used: a newer message replaces the creature's
current one. No stacking, offsetting or queueing has been invented, because
inventing one and calling it 7.72 would be exactly the sort of claim this
project has already had to retract once.

### Positional association: protocol-shaped, resolved in ClientCore

The positional overload writes `Sender`, `Mode`, `x`, `y`, `z`, `Text`. It does
**not** carry a creature id, so the speaker has to be recovered from world
state the client already holds. That recovery is a protocol-adjacent decision
and lives in `clientcore/src/talk_speaker.cpp`, not in Unreal.

### Say versus Whisper: gameplay visibility is the server's

Whether a whisper reaches a given player at all is decided by Fusion32 before
anything is sent — `reference/game/src/operate.cc` chooses the spectators. The
client is told only what it is allowed to know, so no range or filtering rule
is implemented in Unreal. Doing so would be re-implementing a server decision
from an assumption.

Whether the classic client *renders* them differently is a presentation
question and is not established from the binary. Both are shown the same way
here, and that is recorded as unproven rather than styled on a guess.

## The path

```text
Fusion32
  -> SV_CMD_TALK                      (opcode never leaves ClientCore)
  -> DecodeServerUpdate               -> TalkUpdate
  -> ResolveTalkSpeaker(WorldState)   -> creature id, or a reason there is none
  -> FReal33DEvent (worker thread)
  --SPSC queue-->
  -> AReal33DWorld::HandleEvent       (game thread)
  -> AReal33DWorld::PresentSpeech
       -> AReal33DCreature::ShowSpeech   when a speaker was resolved
       -> on-screen fallback line        otherwise
```

The bridge resolves the speaker on the worker thread, where `WorldState` lives,
and what crosses the boundary is a creature id the game thread already keys its
actors by. Unreal never learns the name, the coordinates or the matching rule,
and never searches for a speaker itself.

Every Actor and component touch happens in `PresentSpeech` and
`AReal33DCreature`, both game-thread only and both asserting it.

## Speaker resolution

`ResolveTalkSpeaker` matches on **both** fields and requires exactly one
candidate:

- the creature must be **visible**, meaning it stands on a stored tile. The
  known-creature mirror deliberately retains creatures that scrolled out of
  view, and those have no actor to speak above;
- its position must equal the talk position exactly;
- when the talk carries a name, the creature's name must match.

An empty sender is not a failure: the positional overload's caller in
`reference/game/src/moveuse.cc:941` passes `""` for the `ANIMAL` modes, so the
name is genuinely absent and position alone decides. A field holds one
creature, which is why one player can block another's step, so position is a
sound key on its own.

Outcomes are `Resolved`, `NoMatch`, `Ambiguous` and `NotPositional`. Only
`Resolved` puts text above a creature. Everything else goes to the fallback,
because speech above the wrong creature is a worse failure than speech that is
merely not in the world.

## Fallback

A short on-screen list, newest last, bounded to six lines and expiring on the
same timer. It exists so a message with no provable speaker is still readable
without opening a log, which is part of this milestone's acceptance. It is not
a chat console and is not intended to become one.

## Cleanup

Speech above a creature is a component of that creature's own actor, so a
creature leaving the viewport takes its speech with it. There is no registry
that could outlive an actor and no pointer for anyone else to clear.

`ClearWorld`, which already runs on disconnect and before a reconnect, also
empties the fallback list, so a new session never inherits what the previous
one was saying.

Nothing is stored in `WorldState`. `CHAT-772-001`'s decision stands: Fusion32
keeps no per-connection chat history, so a transcript would be a feature this
client does not have.

## Name colour by health

The creature descriptor and `SV_CMD_CREATURE_HEALTH` both carry a health
figure, and `reference/game/src/crmain.cc::TCreature::GetHealth` shows what it
is: `CurrentHitPoints * 100 / MaxHitPoints`, clamped so a living creature never
reports zero. A percentage, nothing more.

**Fusion32 defines no colours and no thresholds.** Searching the server for any
health-to-colour mapping returns nothing, which places this in the same
category as speech lifetime: the server supplies the number, the client decides
what it looks like.

    HEALTH_COLOUR_OWNER  = CLIENT_PRESENTATION
    HEALTH_COLOUR_PARITY = NOT_PROVEN

The bands are the operator's specification, not 7.72 parity:

| Health | Colour |
| --- | --- |
| 95% and above | green |
| 40% to 94% | yellow |
| 1% to 39% | red |
| 0% | near-black |

Applied when a creature appears and again on every health update, so the colour
tracks damage live.

### A finding worth keeping

The first bands put yellow at 60% and above. A live run showed Player A's name
in red, which looked like a broken health pipeline. It was not: the log said
`creature 1001 "Test Player A" appeared ... health 58%`, so the server was
right, the wiring was right, and 58% was simply falling past yellow into red.

The lesson is the same one this project keeps relearning: a wrong-looking
screen is not evidence of where the fault is. Logging the value turned a
suspected pipeline bug into a two-line threshold correction.

## Two things the live yell established

**The server shouts, not the client.** `operate.cc:2248` calls
`strUpper(YellBuffer)`; the comment at 2168 confirms the text is uppercased only
for `TALK_YELL`. The client sent `now yes please` and the server broadcast
`NOW YES PLEASE`. Casing a yell belongs to Fusion32 and reaches every client, so
REAL33D must not do it and must not undo it.

**Yell has a level gate that looks exactly like a broken client.** `CTalk`
refuses `TALK_YELL` below level 2 and answers `TALK_FAILURE_MESSAGE`, before
`operate.cc` ever selects spectators. For ten sessions no yell reached this
client and nothing was wrong with it. Surfacing `SV_CMD_MESSAGE` is what turned
that from a mystery into a sentence.

    YELL_MIN_LEVEL = 2   (receiving.cc::CTalk)

## Known defect: speech with no provable speaker is unreadable

    DISTANT_SPEECH_READABLE = NO

A yell carries thirty fields but the viewport reaches six, so a distant speaker
is not a visible creature and there is no actor to draw text above.
`ResolveTalkSpeaker` correctly returns `NoMatch` and `PresentSpeech` routes the
line to the fallback rather than guessing at a nearby creature.

The routing is right and the presentation is wrong: the fallback is drawn with
`AddOnScreenDebugMessage`, stacked under the frame and tile counters. The
operator reports not seeing distant yells at all, and the log proves they
arrive. A diagnostics overlay is not where a player reads chat; the classic
client uses a console. A speech area separate from the counters is the fix.

## Operator observations of a chat UI

    EVIDENCE_GRADE = THIRD_PARTY_CLIENT

Weaker than Fusion32 source and weaker than the original binary. The operator
supplied a screenshot of a **different** client, a modern reimplementation, and
described the original's behaviour from memory of playing it. Recorded because
it corroborates choices made blind, never as parity proof. Nothing here
overrides source, and nothing here is cited as a 7.72 rule.

What the screenshot shows:

- Say lines render **yellow** in the default channel. Independent corroboration
  of the colour this client draws, arrived at separately by measuring pixels.
- Line format is `HH:MM Name: text`.
- Channels are tabs: Default, Server Log, RL-Chat, Trade, plus a per-person
  private tab. Consistent with `CTalk` distinguishing channel modes, which carry
  a channel word, from addressed modes, which carry a name.
- A **"Chat off"** toggle exists.

What the operator reports of the original: talk mode is selected by a speaker
control in the corner of the chat panel, and it **persists** until changed.

### Why these two matter

**"Chat off" corroborates a rule this client already enforces.** Typing and
walking are mutually exclusive here, implemented as a single `bComposing` check
at the top of `Request` so that typing "was" cannot walk west, north and south.
That was chosen to avoid a defect, not copied from anywhere. The classic UI
treating it as a first-class, user-visible mode suggests the constraint is
inherent to a client that binds letters to both purposes, rather than an
artefact of this implementation.

**A persistent mode is not what this client does.** F2 and F3 open a one-shot
line and the mode reverts to Say afterwards, so the mode is chosen per message.
The classic affordance sets a mode that stays set. Both produce identical
`CL_CMD_TALK` bytes — the mode byte is the mode byte — so this is presentation,
in the same category as speech lifetime, and equally unprovable from source.

    TALK_MODE_PERSISTENCE = NOT_PROVEN (operator-reported, third-party corroboration)

### The colour drifts with the in-game time of day

A live session showed Say looking greenish while Whisper looked correct, then
both looking correct later, with no code change in between. `PresentSpeech`
never reads the mode and both had resolved to the same creature, so the two were
drawn by the same component in the same colour: a per-mode difference is not
possible. The variable was time, not mode.

Fusion32 sends `SV_CMD_AMBIENT` for the day/night cycle, six times in that
session. The text material is lit, so world light level changes its apparent
hue. Nothing was changed in response, and the observation is recorded here
rather than acted on, because it is another face of the limitation below rather
than a separate defect.

## A known cosmetic limitation: the speech colour is approximate

The operator asked for `#ffff00`. The code sets exactly that, and the screen
shows something warmer and paler. This is recorded rather than quietly left as
if it matched.

What was measured, not assumed: gold `(255,190,30)` reached the screen at
roughly `(180,171,138)` — red and green within nine of each other, which
against the green ground reads as green, which is what the operator reported.

Two causes, both in the renderer rather than in the value:

1. `UTextRenderComponent` defaults to `DefaultTextMaterialOpaque`, which is
   **lit**. The colour is treated as base colour and modulated by the scene's
   warm key light and the sky light's blue ambient, which lifts the blue
   channel of a pure yellow and desaturates it.
2. The filmic tone curve compressed saturation further. That part is fixed: the
   camera now sets `ToneCurveAmount = 0`, which also stops the whole scene
   looking washed out.

The obvious remedy is worse. `/Engine/EngineMaterials/UnlitText` gives the exact
colour but ignores the font texture's alpha, so every character renders as a
filled quad and the text becomes unreadable. That was tried live and rejected
by the operator on sight.

Getting an exact colour **and** readable glyphs needs an authored unlit,
alpha-masked text material. That is content work and belongs with the visual
pipeline, not here. Readable text in an approximate colour is the better state
to stop at for a functional slice.

    SPEECH_COLOUR_EXACT = NOT_ACHIEVED (readable; hue approximate)

## Outgoing: speaking from Unreal

Added by `UNREAL-CHAT-OUTGOING-001`, on top of the incoming presentation above.

### Source trail

`reference/game/src/receiving.cc::CTalk`, reached from `ReceiveData` on
`CL_CMD_TALK` = 150 (`connections.hh:45`):

```text
byte    CL_CMD_TALK             150
byte    Mode
string  Addressee               PRIVATE_MESSAGE, GM_ANSWER, GM_MESSAGE, ANON_MESSAGE
word    Channel                 CHANNEL_CALL, GM_CHANNELCALL, ANON_CHANNELCALL
string  Text
```

`string` is a word length followed by that many bytes, per
`utils.cc::TReadStream::readString`, which also accepts `0xFFFF` as an escape
introducing a quad length; nothing here needs it.

### The client's mode set is not the server's

This is the detail that assuming symmetry would have got wrong, in both
directions:

| Mode | Client may send | Server may send |
| --- | --- | --- |
| `SAY` 1, `WHISPER` 2, `YELL` 3, `PRIVATE_MESSAGE` 4, `CHANNEL_CALL` 5, `GM_REQUEST` 6, `GM_ANSWER` 7, `PLAYER_ANSWER` 8, `GM_BROADCAST` 9, `GM_CHANNELCALL` 10, `GM_MESSAGE` 11, `ANON_CHANNELCALL` 14 | yes | yes |
| `ANONYMOUS_BROADCAST` 13, `ANONYMOUS_MESSAGE` 15 | **yes** | no |
| `HIGHLIGHT_CHANNELCALL` 12, `ANIMAL_LOW` 16, `ANIMAL_LOUD` 17 | no | **yes** |

So a client may say things it can never be told, and be told things it may
never say. `IsClientTalkMode` is therefore a separate table from
`TalkLayoutForMode`, and `TestClientTalkModeSetDiffersFromTheServers` pins both
directions.

### Validation happens before the wire

`BuildTalkCommand` enforces exactly what `CTalk` enforces, so a line the server
would silently discard is refused here with a reason the player can be shown:
empty text, text over 255 bytes (`char Text[256]`, which `readString`
**truncates** rather than rejecting), embedded newlines, a missing addressee,
and an addressee over 29 bytes (`char Addressee[30]`).

The channel number is deliberately **not** validated: `CTalk` checks it against
`GetNumberOfChannels()`, which is server state this client has no copy of.
Guessing a bound would be inventing server knowledge.

### Input

`Enter` opens a line, `Enter` sends it, `Escape` abandons it, `Backspace`
deletes. The composing line is drawn on screen as `say> ...` so the operator is
not typing blind.

While a line is open the walk keys are inert, checked at the top of `Request`
rather than by unbinding and rebinding: one place holds the rule and there is no
window in which the bindings are half-swapped. Typing `was` cannot walk the
player west, north and south.

Characters are bound one key at a time with the character each produces, because
`UInputComponent` reports keys rather than characters and a plain
`APlayerController` has no character event to subscribe to. Lower case, digits
and space are enough to type a test phrase; this is a harness for the protocol
path, not a chat client.

### Known gap: only Say is reachable from the client

The bridge honours the classic `#y ` and `#w ` prefixes and
`BuildTalkCommand` handles every mode `CTalk` accepts, but the input binds only
letters, digits and space. `#` is unbound and needs a modifier on most layouts,
so the operator **cannot type it**: yell and whisper are unreachable from
Unreal despite being implemented and unit-tested beneath.

    OUTGOING_YELL_REACHABLE   = NO (implemented, not reachable from input)
    OUTGOING_WHISPER_REACHABLE = NO (same)

The fix is a key per mode rather than a typed prefix, which needs no modifier
and no punctuation. Left for the next milestone rather than half-finished here.

### Nothing is optimistic

Pressing `Enter` is not proof the server accepted or broadcast anything.
`RequestSay` posts an intent exactly like a walk; Fusion32 decides who hears it,
and the authoritative `SV_CMD_TALK` it broadcasts is what the client draws. No
text is displayed locally on send.

Unreal never constructs a packet: `BuildSayCommand` lives in ClientCore and the
bridge is the only thing that calls it.

## Out of scope

No chat console, channel tabs, private-message windows, NPC conversation UI,
persistent history, or final typography. The presentation is deliberately plain
and replaceable.
