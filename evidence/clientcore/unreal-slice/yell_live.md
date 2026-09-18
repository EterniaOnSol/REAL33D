# Yell, finally delivered

Yell had never once reached this client: ten sessions, zero `talk [Yell]`.
Every attempt was refused by Fusion32 because the speaker was level 1, which
`receiving.cc::CTalk` rejects before `operate.cc` reaches the spectator loop.

With Test Player B raised to level 2 the whole chain works, with **zero**
refusals:

```
talk mode is now yell
yell 1: "now yes please" handed to Fusion32
talk [Yell] at 32097,32213,7, Test Player B: "NOW YES PLEASE"  speaker Resolved creature 1002
```

Operator confirmation, HUMAN: Player A read it in the original Tibia.exe.

## The server shouts, not the client

The client sent `now yes please` and the server broadcast `NOW YES PLEASE`.
`operate.cc:2248` calls `strUpper(YellBuffer)`, and the comment at 2168 says the
text "is only modified when a player uses `TALK_YELL` to make it upper". Casing
a yell is Fusion32's behaviour, reaching every client. Nothing in REAL33D does
it, and nothing in REAL33D should.

## The fallback path, exercised live for the first time

```
talk [Yell] at 32098,32204,7, Test Player A: "YOU SURE?"  speaker NoMatch creature 0
```

A was nine fields north of B and the viewport reaches six, so A was not a
visible creature and there was no actor to draw text above. `ResolveTalkSpeaker`
returned `NoMatch` and `PresentSpeech` sent the line to the fallback instead of
attributing it to whichever creature happened to be nearby. That is the policy
working: speech above the wrong creature is a worse failure than speech that is
merely not in the world.

## Open defect: the fallback is unreadable

    DISTANT_SPEECH_READABLE = NO

The operator reports that B cannot see what A yells, and the log proves the text
arrived and was routed correctly. `DrawOverlay` runs every tick with no guard
and adds the line in yellow, but it does so with
`AddOnScreenDebugMessage`, stacked underneath the frames, tiles and anchor
counters. Whether it is invisible or merely buried is not yet established: the
outstanding question is whether the operator sees the diagnostic counters at
all.

The design is wrong either way. Speech with no provable speaker still has to be
readable, and a diagnostics overlay is not where a player reads chat. The
classic client puts exactly this in a chat console. A real speech area,
separate from the counters, is the fix.
