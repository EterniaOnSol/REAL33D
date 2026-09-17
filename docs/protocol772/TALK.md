# Talk

`SV_CMD_TALK`, opcode 170. Task `CHAT-772-001`.

Until this milestone the command was recognised by name and not decoded, which
stopped the frame walk at the first thing anyone said and left the rest of the
payload as residual bytes. It was the sole remaining qualification on
`UNREAL-SLICE-001` criterion 12.

## Source trail

| What | Where |
| --- | --- |
| Opcode `SV_CMD_TALK = 170` | `reference/game/src/connections.hh:120` |
| The three serializers | `reference/game/src/sending.cc:1328`, `:1365`, `:1402` |
| `SendString` = word length + bytes | `reference/game/src/sending.cc:149` |
| `TALK_MODE` values | `reference/game/src/enums.hh:651` |
| A different opcode for modes 18..23 | `reference/game/src/sending.cc:1618`, `SendMessage` |

Nothing here is taken from modern Tibia documentation or another server.

## The packet

Three overloads of `SendTalk` all emit opcode 170, and all begin identically:

```text
byte    SV_CMD_TALK             170
quad    StatementID             uint32 little-endian
string  Sender                  uint16 length, then that many bytes
byte    Mode                    TALK_MODE
```

They then differ, and **the mode is the only thing that says how**. The server
picks an overload by argument types at the call site; the client has to
recover that choice from the mode alone.

### Positional, `SendTalk(..., int x, int y, int z, const char *Text)`

```text
word    x
word    y
byte    z
string  Text
```

Modes `TALK_SAY` 1, `TALK_WHISPER` 2, `TALK_YELL` 3, `TALK_ANIMAL_LOW` 16,
`TALK_ANIMAL_LOUD` 17.

This is ordinary in-world speech, and it is the form a player typing in the
game window produces.

### Channel, `SendTalk(..., int Channel, const char *Text)`

```text
word    Channel
string  Text
```

Modes `TALK_CHANNEL_CALL` 5, `TALK_GAMEMASTER_CHANNELCALL` 10,
`TALK_HIGHLIGHT_CHANNELCALL` 12, `TALK_ANONYMOUS_CHANNELCALL` 14.

`TALK_ANONYMOUS_CHANNELCALL` sends an **empty** sender rather than omitting the
field:

```cpp
if(Mode != TALK_ANONYMOUS_CHANNELCALL){
    SendString(Connection, Sender);
}else{
    SendString(Connection, "");
}
```

A decoder that treated anonymity as an absent field would lose two bytes and
desynchronise. The field is always present; it is the contents that are blanked.

### Plain, `SendTalk(..., const char *Text, int Data)`

```text
quad    Data                    only when Mode == TALK_GAMEMASTER_REQUEST
string  Text
```

Modes `TALK_PRIVATE_MESSAGE` 4, `TALK_GAMEMASTER_REQUEST` 6,
`TALK_GAMEMASTER_ANSWER` 7, `TALK_PLAYER_ANSWER` 8,
`TALK_GAMEMASTER_BROADCAST` 9, `TALK_GAMEMASTER_MESSAGE` 11.

`TALK_GAMEMASTER_REQUEST` is the one mode anywhere in this command that inserts
a field between the mode and the text. Everything else in this form goes
straight to the text.

## Modes that cannot arrive

`enums.hh` declares more `TALK_MODE` values than `SendTalk` accepts, and the
difference matters because an unrecognised mode makes the tail unlocatable.

- `TALK_ANONYMOUS_BROADCAST` 13 and `TALK_ANONYMOUS_MESSAGE` 15 are declared
  and accepted by **no** overload. Each `SendTalk` opens with an explicit mode
  whitelist and `error()`s out otherwise, so these never reach the wire under
  this opcode.
- 18 to 23 belong to `SendMessage`, which is a different command entirely and
  is already decoded as `ServerUpdateKind::Message`.

The decoder therefore refuses an unknown mode with `UnknownTalkMode` and
consumes nothing, rather than guessing a length. That is the same policy the
layer already applies to an unsized opcode: stopping is honest, guessing
corrupts everything after it.

## Semantic result

`ServerUpdateKind::Talk` with a `TalkUpdate`:

| Field | Meaning |
| --- | --- |
| `statement_id` | the server's id for this utterance |
| `speaker` | sender; empty for anonymous channel calls and the animal modes |
| `mode` | raw `TALK_MODE`, with `TalkModeName` for a readable form |
| `layout` | `Positional`, `Channel` or `Plain`, derived from the mode |
| `text` | the message |
| `has_position`, `position` | positional forms only |
| `has_channel`, `channel` | channel forms only |
| `has_request_data`, `request_data` | `TALK_GAMEMASTER_REQUEST` only |

The optional fields are flagged rather than sentinel-valued, so a caller cannot
mistake "absent" for "zero".

## Ownership: talk stores nothing

`ApplyServerUpdate` deliberately does not touch `WorldState` for talk.

`WorldState` mirrors what the server keeps about the world. Fusion32 keeps no
per-connection chat history: `SendTalk` serialises and forgets. A client-side
transcript would be a feature this client does not have, and holding one in
`WorldState` would make it look like server state that could be compared
against a fresh `FULLSCREEN`. Talk reaches the caller as an event and ends
there, exactly like the graphical and textual effects.

If a chat window is built later, it owns its own history.

## Tests

`clientcore/tests/movement_tests.cpp`. The emitter in
`fixtures/fullscreen_772_vectors.h` is a literal port of all three overloads,
asserted against hand-computed golden bytes before any structural test uses it.

- `TestGoldenTalk` — one golden per form, byte for byte.
- `TestTalkFormsDecode` — every mode each overload accepts, asserted through
  semantic fields only.
- `TestTalkEmptyAndLongText` — empty text, empty speaker as the animal modes
  actually send, and a 600-byte message.
- `TestTalkKeepsTheStreamAligned` — three talks then a move and a ping,
  decoded in sequence to exactly zero residual bytes.
- `TestTalkNegativeCases` — every truncation length, unreachable and unknown
  modes, and a text length that runs past the buffer.
- `TestTalkLayoutClassification` — the mode-to-layout table asserted directly.
- `TestTalkDoesNotTouchWorldState` — tiles, creatures and anchor unchanged.

## Not in scope

No chat UI, no window, no text above creatures. This milestone decodes the
command and surfaces it; `UNREAL-SLICE-001`'s presentation boundary is
unchanged and the Unreal module still contains no protocol knowledge. The
bridge resolves the mode to a name so nothing above it sees a mode number.
