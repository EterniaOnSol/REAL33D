# Talk mode persists, and the server's refusal is visible

F2 cycles say -> whisper -> yell. The mode survives sending:
```
talk mode is now yell
talk mode is now yell
talk mode is now yell
talk mode is now yell
yell 131: "hello" handed to Fusion32
yell line opened; walk keys are inert
```

Two yells were sent with F2 pressed once, before the first.

Neither was broadcast, and the reason is now visible rather than silent:
```
server message [FailureMessage]: You may not yell as long as you are on level 1.
```

receiving.cc::CTalk refuses TALK_YELL when the speaker is level 1, before
operate.cc ever reaches the spectator loop. Test Player B is level 1, so
Fusion32 was right and the client drew nothing because nothing was sent.

This looked like "yell is broken" until the client surfaced SV_CMD_MESSAGE.
The refusal had been arriving all along, decoded and discarded.
