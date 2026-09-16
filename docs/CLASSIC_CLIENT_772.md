# Classic Tibia 7.72 Client Gate

Status: `BLOCKED_CLIENT_ABSENT`.

| Requirement | Current value |
| --- | --- |
| Version | Tibia 7.72 desktop client |
| Expected executable | legitimate exact 7.72 executable; filename and provenance `UNKNOWN` until supplied |
| Executable SHA-256 | `UNKNOWN` |
| Login host | `127.0.0.1` for local WSL testing |
| Login port | `7171` |
| Game endpoint | returned by Login from world row: `127.0.0.1:7172` |
| RSA public modulus | must match the fresh local runtime key; read from `/tmp/fusion32-server-baseline-772-$UID/secrets/public-modulus.hex` inside WSL |
| IP Changer | source candidate supports 7.72 host/port/modulus patching; historical binary must not be executed |

Because the sanitized server uses a fresh key, the client must use its matching public modulus. A controlled client-memory patch or a newly built/audited IP Changer is therefore expected; exact executable offsets and live behavior remain `UNKNOWN`. The historical private key is not reused.

When a legitimate client is supplied: hash and record provenance, verify version, validate exact patch addresses, patch host/port/modulus, authenticate with one generated synthetic identity, capture character-list evidence, select the character, and prove Game `JoinGame` plus initial world data. Never commit the executable, runtime passwords or private key by default.
