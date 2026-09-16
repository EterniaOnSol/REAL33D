# Protocol772Core transport and outer framing

Task: `CLIENTCORE-TRANSPORT-772-001`

Status: `PASS`

Scope: portable TCP lifecycle, byte I/O, incremental outer framing and deterministic tests. RSA, XTEA, protocol opcodes, semantic events, WorldState and Unreal are deliberately outside this task.

## Layer boundary

```text
TCP stream
    -> TcpTransport
    -> FrameDecoder / EncodeFrame
    -> FramedPacket (owned outer payload)
    -> CryptoStage (implemented by CLIENTCORE-CRYPTO-772-001)
    -> future Protocol772 decoder
    -> future semantic event queue
```

`TcpTransport` does not know packet boundaries. `FrameDecoder` does not decrypt or interpret opcodes. `FramedConnection` composes those two layers without introducing gameplay or Unreal dependencies. A framing error is terminal until explicit `Reset`; bytes after the offending header are returned as `unconsumed_bytes` and are never silently discarded.

## Canonical source trace

All paths below are under the selected immutable `reference/` trees. Line numbers identify the current selected revisions and symbols are the durable locator.

| Component | Source / symbol | Observed rule | 7.72 relevance | Confidence |
| --- | --- | --- | --- | --- |
| Game input | `reference/game/src/communication.cc::ReceiveCommand`, lines 1122-1279 | Reads a two-byte header, decodes `Help[0] | Help[1] << 8`, rejects zero or a value greater than `sizeof(InData)`, then reads exactly that payload count | The outer framing is outside the `TIBIA772` guards | High |
| Game input buffer | `reference/game/src/connections.hh::TConnection::InData`, line 195 | Input payload buffer is 2,048 bytes | No `TIBIA772` conditional | High |
| Game stream read | `reference/game/src/communication.cc::ReadFromSocket`, lines 620-649 | Repeats `read` until the requested count, peer close, timeout or error | No `TIBIA772` conditional | High |
| Game encrypted phase | `reference/game/src/communication.cc::ReceiveCommand`, lines 1247-1277 | After login, outer payload must be a multiple of eight; XTEA plaintext begins with a two-byte LE inner data length | No framing change under `TIBIA772`; this is the future crypto boundary | High |
| Game output | `reference/game/src/communication.cc::GetPacketSize`, `WriteToSocket`, `SendData`, lines 250-325 and 373-409 | Total wire layout is two-byte outer length plus XTEA blocks; outer length is `Size - 2`, excludes its own header, and includes encrypted inner length/data/padding | No `TIBIA772` conditional | High |
| Game output buffer | `reference/game/src/connections.hh::TConnection::OutData`, line 199 | Ring buffer can contribute up to 16,384 data bytes to `GetPacketSize` | No `TIBIA772` conditional | High for the selected source-derived bound |
| Login endian helper | `reference/login/src/common.hh::BufferRead16LE`, lines 150-153 | Two-byte values are little-endian | No `TIBIA772` conditional | High |
| Login input | `reference/login/src/connections.cc::CheckConnectionInput`, lines 157-247 | Incrementally reads two header bytes and then the declared payload; rejects zero or a value over `sizeof(Buffer)` | The framing path is outside the `TIBIA772` guards | High |
| Login shared buffer | `reference/login/src/common.hh::TConnection::Buffer`, line 575 | Shared input/output buffer is 2,048 bytes | No `TIBIA772` conditional | High |
| Login output | `reference/login/src/connections.cc::PrepareXTEAResponse`, `SendXTEAResponse`, lines 503-550 | Reserves outer and inner two-byte lengths in the 2,048-byte buffer; outer length excludes its own two bytes and covers the encrypted block | No `TIBIA772` conditional | High |
| Login version gate | `reference/login/src/connections.cc::TERMINALVERSION`, lines 10-14 | `TIBIA772` selects version 772 instead of 770 | Changes accepted version, not outer framing | High |
| Game login layout | `reference/game/src/communication.cc::HandleLogin`, lines 920-952 | `TIBIA772` moves terminal type/version outside the RSA block | Changes login payload layout, not outer framing | High |

Query Manager was inspected only as an architectural comparison. Its internal service protocol is not authority for the Tibia-facing client framing implemented here.

## Wire interpretation

- Header: unsigned 16-bit little-endian.
- Header width: two bytes.
- Declared size: payload bytes following the header; it excludes the header itself.
- Zero length: malformed and terminal for the decoder.
- Incomplete header or payload: retained until more TCP bytes arrive.
- Multiple frames in one TCP read: extracted in order.
- Complete frame plus partial next frame: complete frame is returned and partial bytes remain buffered.
- End of stream with buffered bytes: `TruncatedFrame`.
- Bytes supplied after clean end of stream: `StreamFinished`, with all new bytes unconsumed.
- Declared size beyond the selected endpoint/direction profile: `ExceedsConfiguredMaximum`, retaining the offending header and returning later bytes unconsumed.

The client-facing profiles are deliberately directional:

| Profile | Maximum received outer payload | Maximum sent outer payload | Derivation |
| --- | ---: | ---: | --- |
| Login | 2,046 | 2,048 | Server response storage is 2,048 bytes total including the two-byte outer header; server input accepts an outer payload up to its 2,048-byte buffer |
| Game | 16,392 | 2,048 | `GetPacketSize(16,384)` produces 16,394 total wire bytes, hence 16,392 after the outer header; server input is capped by `InData[2048]` |

The 65,535-byte representational limit is imposed by the wire header. It is not used as an endpoint profile where the selected Fusion32 source establishes a smaller bound.

## Lifecycle and error behavior

`TcpTransport` is move-only and exposes `Disconnected`, `Connecting`, `Connected`, `RemoteClosed` and `Failed`. It resolves IPv4/IPv6 addresses, performs a bounded nonblocking connect, restores blocking mode, enables `TCP_NODELAY`, and applies receive/send timeouts. Reads return whatever bytes TCP currently provides. Writes loop until all bytes have been sent; if an error occurs after a partial write, the connection is closed because the higher layer cannot safely reconstruct stream synchronization.

A read timeout is non-terminal and preserves any frame bytes already buffered by `FramedConnection`. A remote FIN becomes `RemoteClosed`; `FramedConnection` then calls `FrameDecoder::Finish` so a partial final frame cannot be mistaken for a clean disconnect. `Connect` and `Disconnect` reset framing state for a new lifecycle.

## Parser-safety contract

This layer owns framing only. It never drops the remainder of an outer packet because of an unknown future opcode. Each `FramedPacket` owns its complete payload, so the later protocol decoder can distinguish `DECODED`, `INCOMPLETE`, `UNSUPPORTED`, `MALFORMED` and `PROTOCOL_VIOLATION` without affecting TCP synchronization. Those protocol outcomes themselves are not implemented in this task.

## Validation and limitations

The deterministic suite covers fragment boundaries, coalesced frames, malformed lengths, preserved bytes, write framing, local connection/refusal, clean and remote disconnects, reconnect, and partial-frame EOF. It runs without Internet, Fusion32 services, the classic client or Unreal. Debug and AddressSanitizer/UBSan builds pass on WSL2 Linux x86_64.

Transport-specific items not yet verified:

- a native Windows/MSVC build and Windows loopback run;
- a live Fusion32 transport smoke test (optional for this task and intentionally not run);
- login messages, opcodes or application semantics; RSA/XTEA now have their own bounded `PASS` in `docs/protocol772/CRYPTO.md`;
- use of optional server-side local-proxy preambles (`ALLOW_LOCAL_PROXY`), which a normal direct client does not send;
- independent repetition of this task's test procedure.
