# REAL33D-CHAT-SEND-FIX

State: `IMPLEMENTED_UNVERIFIED` pending live retest.

Operator report: action-bar text sent successfully, but typed chat remained in
the visible input and neither Enter nor Send transmitted it.

Live failure in `unreal/REAL33D/Saved/Logs/REAL33D.log`, 2026-09-24 UTC:

- 02:50:27 connected as Test Player B.
- 02:54:35 a walk request was accepted by Fusion32; HP updates continued.
- 02:54:36 `chat input active; movement keys are inert`.
- 02:54:37, 02:54:43, 02:54:45 `not connected; "hi" not sent`.

The session was not disconnected. `SReal33DChatPanel::Send` tests its own
`Bridge`, and `SReal33DHUD::MakeBottomPanel` constructed the embedded panel
with `.Embedded(true)` but omitted `.Bridge(Bridge.Get())`. The action slots,
which call the HUD's Bridge directly, therefore sent TALK successfully.

Change: pass the existing Bridge to the chat panel. No TALK packet, server,
WorldState or reconnect logic changed.

Build: `REAL33DEditor Win64 Development` PASS (4 incremental actions).

Pending: reopen, type a unique Say
message in chat, press Enter and then test Send, confirm `TalkId` and the
authoritative echoed speech in the transcript/log, and verify movement keys
work again after committing the line. Record protocol counters at close.
