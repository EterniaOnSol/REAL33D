# HANDOFF

Date/time: 2026-09-17
Agent: Claude
Role: VISIBLE CHAT IN UNREAL
Branch: `main`
Starting commit: `5012d93`
Implementation commit: `cf034eb`
Worktree: clean after the focused commit
Remote: `origin` = `https://github.com/EterniaOnSol/REAL33D.git`, `HEAD == origin/main`

Previous handoff archived at `handoffs/archive/2026-09-17_CHAT-772-001.md`.

## Objective

`UNREAL-CHAT-PRESENTATION-001`: make the speech `CHAT-772-001` decodes actually
visible to the operator in Unreal, deriving behavioural ownership from source
rather than inventing it.

## Result

`QUALIFIED PASS`. Full record in
`evidence/clientcore/UNREAL-CHAT-PRESENTATION-001.md`; design and ownership in
`docs/UNREAL_CHAT.md`.

Visible incoming Say is proven live and confirmed by the operator. Whisper takes
the identical path and decodes live but was **not** separately confirmed visible
in this milestone, so it is not claimed.

## Ownership, proven as a negative

No `SendTalk` overload carries a duration, the complete server command list has
**no talk-removal command**, and `StatementID` is a moderation log id from
`LogCommunication`. So lifetime is neither server nor protocol owned:

    SPEECH_LIFETIME_OWNER = CLIENT_PRESENTATION

But `reference/` holds only server sources and the classic client exists here as
a binary, so the rule itself is unprovable:

    SPEECH_LIFETIME_PARITY = NOT_PROVEN
    CONSECUTIVE_MESSAGE_PARITY = NOT_PROVEN

A 6 s placeholder is used, overridable with `-real33d-speech-seconds=`, and the
client logs that the value is not a proven 7.72 value on every startup.

## Speaker resolution

`ResolveTalkSpeaker` in ClientCore requires exactly one **visible** creature
matching the talk position, and the name too when present. Only `Resolved` puts
text above a creature; `NoMatch`, `Ambiguous` and `NotPositional` go to a
visible fallback, because speech above the wrong creature is worse than speech
that is merely not in the world. Unreal receives a creature id, never the rule.

## Tests

transport 22/22, crypto 25/25, the six named suites PASS, Windows MSVC at C++17
and C++20 `WINDOWS CLIENTCORE: PASS`, WSL ASan/UBSan 8/8. Five new cases cover
the resolution policy, including one asserting that a right name at the wrong
position and a right position with the wrong name both resolve to nothing.

## Known cosmetic limitation

The operator asked for `#ffff00`; the code sets it and the screen shows
something warmer. Measured: gold `(255,190,30)` arrived at about
`(180,171,138)`. `UTextRenderComponent` defaults to a **lit** material, so the
colour is modulated by the key light and the sky light's blue ambient; the
filmic tone curve compressed saturation further and is now disabled.
`/Engine/EngineMaterials/UnlitText` gives the exact colour but ignores the font
alpha and renders every glyph as a filled block, which was tried live and
rejected on sight. Readable text in an approximate colour was chosen.

    SPEECH_COLOUR_EXACT = NOT_ACHIEVED (readable; hue approximate)

Fixing it properly needs an authored unlit, alpha-masked text material, which is
content work for the visual pipeline.

## Not done, deliberately

    OUTGOING_UNREAL_CHAT = NOT_IMPLEMENTED

No chat console, channel tabs, private-message windows, NPC conversation UI,
persistent history or final typography. Nothing is stored in `WorldState`:
`CHAT-772-001`'s decision stands.

## Suggested next milestone, not started

`UNREAL-CHAT-OUTGOING-001`, letting B speak from Unreal through a semantic
ClientCore action, is the natural other half and was deliberately left out here.

Alternatives: `CONTAINERS-772-001` or `TRADE-772-001`;
`ROOKGAARD-P0-MOCKUPS-001`, unblocked because the asset registry can adopt
approved art without touching ClientCore or any Actor; or a floor-transition
slice, still the one movement case the 3D client has never exercised.
