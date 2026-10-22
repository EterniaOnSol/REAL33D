# The defect, as captured before the fix

Two REAL33D clients logging in on the same character, the second arriving while
the first was still online, so Fusion32 routed it to `TPlayer::TakeOver`. Both
clients reported, verbatim:

```
LogReal33D: Warning: client core: SV_CMD_INIT_GAME is not decoded at offset 1 of 1921;
1920 bytes of the payload left unwalked; head=A3 0A EA 03 00 00 32 00 00 64 58 7D
```

Reading the head:

| bytes | meaning |
|---|---|
| `A3` | 163, `SV_CMD_CLEAR_TARGET`, zero body — one byte |
| `0A` | 10, `SV_CMD_INIT_GAME`, at **offset 1** |
| `EA 03 00 00` | creature id 1002, little-endian |
| `32 00` | beat 50 |
| `00` | bug reports |
| `64` | 100, `SV_CMD_FULLSCREEN` — the initial world |

The offset and the `head=` window are not in the shipped diagnostic; they were
added to find this and the offset was kept, because "is not decoded" without a
position cannot distinguish "the server sent something new" from "this client
lost its place earlier in the same frame".

The second client never reached the world: `1920 bytes left unwalked` is the
whole of `SV_CMD_FULLSCREEN`, so there were no tiles and every walk it sent was
refused or expired unanswered.
