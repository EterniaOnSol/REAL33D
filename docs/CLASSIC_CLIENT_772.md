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
| RSA public modulus | must match the fresh local runtime key; Fusion32 IP Changer requires the decimal form generated at `/tmp/fusion32-server-baseline-772-$UID/secrets/public-modulus.decimal` |
| IP Changer | required Fusion32 source revision `8215db18...`, materialized at `reference/ipchanger`; Windows x86 `BUILD PASS` |

Because the sanitized server uses a fresh key, the client must use its matching public modulus. The project will use only the Fusion32-authored IP Changer built from the selected source. The historical private key and any archived IP Changer binary are not used.

## Fusion32 7.72 address table

| Field | Source-defined value |
| --- | --- |
| Version marker | `Version 7.72` at `0x55C65D` |
| Login endpoints | 5 entries, stride 112 bytes |
| First hostname | `0x7152F8`; maximum 100 bytes including terminator |
| First port | `0x71535C`; written as four little-endian bytes |
| RSA modulus | `0x55B620`; maximum 312 bytes including terminator |

The tool locates a window of class `TibiaClient`, validates the version marker, patches all five login endpoints, temporarily makes the read-only RSA area writable, writes the decimal modulus string, and restores protection. These addresses are authoritative source claims but remain live-unverified until matched to the legitimate executable.

## Reproducible preparation

From a Visual Studio x86 developer environment:

```bat
tests\build_ipchanger_windows.cmd
```

Generate the runtime-specific `servers.txt` without printing the modulus:

```powershell
wsl.exe -d Ubuntu-26.04 -- bash /mnt/c/Users/dell/Desktop/fusion32/scripts/client/prepare_fusion32_ipchanger_wsl.sh /mnt/c/Users/dell/Desktop/fusion32
```

This creates the ignored entry `772;fusion32;127.0.0.1;7171;<decimal modulus>` in `build/ipchanger/servers.txt`. Once the legitimate client is running and its hash/version are confirmed, run the newly built `build/ipchanger/ipchanger.exe fusion32` from that directory. Do not perform this live step against an unknown executable.

When a legitimate client is supplied: hash and record provenance, verify version, validate the Fusion32 address table, use the generated `fusion32` entry, authenticate with one generated synthetic identity, capture character-list evidence, select the character, and prove Game `JoinGame` plus initial world data. Never commit the executable, generated `servers.txt`, runtime passwords or private key by default.
