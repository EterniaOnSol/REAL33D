# The distant yell is readable

    UNREAL-CHAT-AREA-001 = PASS
    DISTANT_SPEECH_READABLE = YES  (read in the chat panel)

Record: `chat_area_live.log`. Operator confirmation, HUMAN: both yells were read
in the chat panel.

The defect this milestone existed to close was proven on 2026-09-18 and is
recorded in `yell_live.md`: a yell from a speaker outside the viewport arrived
correctly and was routed correctly, and the operator still could not read it,
because the fallback drew it with `AddOnScreenDebugMessage` underneath the
diagnostic counters. A diagnostics overlay is not where a player reads chat.

Both paths were exercised in a single run, which is what makes the result worth
keeping. One case alone would not have distinguished a working chat area from a
chat area that quietly attributes distant speech to whatever creature is nearby.

```
talk [Yell] at 32097,32208,7, Test Player A: "AAAAMOOOOOOR DIVIIIINOOOOO, ..."  speaker NoMatch  creature 0
talk [Yell] at 32097,32219,7, Test Player B: "YA ACEPTALO AWEONAO, ..."          speaker Resolved creature 1002
```

**Distant, unresolved.** A was at `32097,32208` and B at `32097,32219`: eleven
fields apart on the same floor, against a viewport that reaches six.
`ResolveTalkSpeaker` returned `NoMatch` and there was no actor to draw text
above. The line was still read. `PresentSpeech`
(`Real33DWorldActor.cpp:590-596`) draws nothing in this case and only counts
`SpeechInChatArea`; what makes the text readable is `NoteChat`
(`Real33DBridge.cpp:893-912`), which files every utterance in the transcript as
it is drained, whether or not a speaker resolved.

**Nearby, resolved.** B's own yell resolved to creature `1002` and was also
filed in the transcript. Adding the chat area did not displace the resolved
path.

The server uppercased both texts. `operate.cc:2248` calls `strUpper(YellBuffer)`
and the comment at 2168 says the text "is only modified when a player uses
`TALK_YELL` to make it upper". Nothing in REAL33D uppercases anything.

Zero refusals. `receiving.cc::CTalk` enforces `YELL_MIN_LEVEL = 2` before
`operate.cc` reaches the spectator loop, and both characters were level 2 for
the whole run.

## What this run does not prove

    IN_WORLD_SPEECH_READABLE = NOT_PROVEN
    SPEECH_LIFETIME_PARITY = NOT_PROVEN
    CONSECUTIVE_WORLD_SPEECH_PARITY = NOT_PROVEN
    EXACT_SPEECH_COLOR_PARITY = NOT_PROVEN
    TALK_MODE_PERSISTENCE = REAL33D_UI_BEHAVIOUR, not proven parity
    CHAT_HISTORY_CAPACITY = REAL33D_UI_BEHAVIOUR, not 7.72 parity

`IN_WORLD_SPEECH_READABLE` is new here and must not be folded into the PASS
above. The operator read both lines **in the chat panel only**, and reports
wanting to read the resolved one above the speaker as well. The resolved path
ran — the log proves the routing — but that the `SpeechTag` text above creature
`1002` was legible on screen is not something this run established, and it is
not what the milestone claimed.

The obstacle is not the text component. `AReal33DCreature` draws speech as a
`UTextRenderComponent` above the name tag, rescales it per frame against camera
distance, and turns it to face the camera every tick
(`Real33DCreatureActor.cpp:190-198`). Two things work against reading it, and
they pull in opposite directions:

**The tag billboards in yaw only.** `FRotator(0.0, ViewRotation.Yaw + 180.0,
0.0)` leaves the text standing upright, so a camera looking down sees it
foreshortened — about 74% of its height at the default -42 degrees, and a sliver
approaching straight down. Lowering the camera improves it.

**There is no horizon to lower the camera towards.** `kTerminalWidth = 18` and
`kTerminalHeight = 14` (`worldstate.h:19-20`) are the whole world this client is
told about: the protocol sends a window roughly nine fields in each direction
and nothing beyond it. What is drawn is a slab that travels with the player, so
a shallow camera looks off its edge into empty space. This is a property of the
7.72 protocol, not a loading delay and not something the renderer can be asked
to fix. Giving the client a horizon means reading the server's `.sec` sector
files from `<runtime>/game/state/map` instead of relying on the protocol window,
which is a separate piece of work.

An orbit camera was added on 2026-09-19 so the angle is at least the operator's
to choose: right mouse button held plus mouse movement, wheel to zoom, limits
owned by `AReal33DWorld`. It compiles clean and its defaults reproduce the
previous fixed view exactly. **It has not been exercised in a live run**, and
whether it makes in-world speech readable is precisely the open question above.

Readability was confirmed by the operator reading the screen. Nothing here
measures how long a line stays, how the panel behaves under a burst of
simultaneous speech, or whether its colours match the classic client.

## Runtime note

This run is also the first on the relocated runtime. Every earlier attempt at
this milestone was undermined by the runtime living on `tmpfs`; see
`docs/SERVER_RUNTIME.md`, section "Why `/var/lib` and not `/tmp`". The level-2
state both speakers needed had been applied on 2026-09-18 and was gone by
2026-09-19 for that reason alone.
