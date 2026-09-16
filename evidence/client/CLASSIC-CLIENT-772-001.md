# CLASSIC-CLIENT-772-001 Evidence

Date: 2026-09-15
Environment: Windows desktop client against the sanitized WSL2 Fusion32 runtime
Aggregate status: `IN_PROGRESS`

The exact local client passed static identity/address checks and completed Login, character selection, Game entry, initial world display and a sustained Game connection. The historical acquisition source of the operator's pre-existing local copy is `UNKNOWN`, and no independent reviewer has repeated the live test. The aggregate gate is therefore not `CERTIFIED`.

## Preconditions

- `SERVER-RUNTIME-SMOKE-001 = CERTIFIED` for the selected Game/Login/Query Manager runtime.
- Game and Login were built with `TIBIA772=1` and used a fresh generated 1024-bit RSA key.
- The selected Fusion32 IP Changer was revision `8215db18abbae05b62bcbd5c4f086856168283a4`, built locally as Windows x86 from `reference/ipchanger`.
- The runtime-specific ignored `servers.txt` supplied `127.0.0.1:7171` and the matching generated decimal public modulus.
- The client executable and data were an operator-supplied copy that already existed on the same computer. No original URL, installer, archive, acquisition date or chain of custody is available. Windows Alternate Data Streams contained no `Zone.Identifier`.
- Only generated synthetic account/character data was used. No password, private key, generated modulus or full runtime log is retained here.

## Selected local artifact identity

The artifacts remain ignored under `build/classic-client-772/app/` and are not committed.

| Artifact | Bytes | SHA-256 |
| --- | ---: | --- |
| `Tibia.exe` | 1,875,968 | `1A82FE97A39E536C29327D6D5DDE316775F537DCC01506831429B1A0433B76F8` |
| `Tibia.dat` | 175,805 | `3C5E857FF72FD1E52879EB8845ADC72C0991786AD0EAF85F6847595C9677AEDD` |
| `Tibia.spr` | 15,877,515 | `86ABBF5FADCF84313A03615A55A8BD626BAAB98671F0A78C9B36CB61CCE0C64A` |
| `Tibia.pic` | 1,360,102 | `0C9587C12965C6243F7037BF7B72B102B2214D3BBDB4A7EC26A01AB16E6D833D` |

PE resource metadata reported `Tibia Player`, file/product version `7.72`, company `CipSoft GmbH`, original filename `Tibia.exe`. This supports artifact identity; it does not establish historical provenance or licensing.

## Static address-table test

Command:

```powershell
wsl.exe -d Ubuntu-26.04 -- python3 /mnt/c/Users/dell/Desktop/fusion32/tests/verify_classic_client_772.py /mnt/c/Users/dell/Desktop/fusion32/build/classic-client-772/app/Tibia.exe
```

Expected: all four hashes match the selected local set; PE is x86/PE32; embedded version starts with `Version 7.72`; all five source-defined endpoint ranges are writable `.data`; the source-defined RSA address contains a bounded decimal modulus.

Observed:

```text
CLIENT_ARTIFACT Tibia.exe ... PASS
CLIENT_ARTIFACT Tibia.dat ... PASS
CLIENT_ARTIFACT Tibia.spr ... PASS
CLIENT_ARTIFACT Tibia.pic ... PASS
PE machine=0x014C image_base=0x400000 timestamp=0x4487D508 PASS
VERSION_ADDRESS 0x55C65D section=.rdata value='Version 7.72 ...' PASS
ENDPOINT_0..4 source-defined .data addresses PASS
RSA_ADDRESS 0x55B620 section=.rdata chars=309 PASS
FUSION32_IPCHANGER_STATIC_ADDRESS_TABLE PASS
```

Result: `CLASSIC-CLIENT-772-STATIC-001 = PASS`.

## Live test and observations

The operator launched this exact `Tibia.exe`, applied the freshly generated `fusion32` entry with the locally built Fusion32 IP Changer, authenticated with the generated synthetic identity, selected `Test Player A`, entered the world and left the client connected.

Independent read-only inspection during that same live session established:

- Query Manager, Game and Login were alive with verified executable, cwd, start token and listener ownership on `7173`, `7172` and `7171`.
- Login advertised server/client version `7.72`; its log recorded the live client connections beginning at `2026-09-15 19:40:46-06:00`.
- Game logged `Spieler Test Player A loggt ein ...` and the first-login path `Outfitwahl`, followed by player quest-state activity.
- The client produced map-cache files at `19:47:51` and `19:50:02`, corroborating initial world/map processing.
- Windows showed `Tibia.exe` PID 7680, started at `19:40:27`, with an `ESTABLISHED` TCP connection from loopback to Game `127.0.0.1:7172` during repeated checks after 20:07 and again after 20:11.
- The observed sustained interval exceeded 30 minutes from client process start; the client and Game connection were still active at the final check.

The successful Login-to-Game transition is live validation of the IP Changer address table and fresh public-modulus patch for this exact process image: the client reached the local endpoints and completed the RSA/XTEA login paths. This is an evidence-backed inference from the source-defined patch behavior plus the observed server/client connection; no process-memory dump containing the generated modulus was retained.

## Results

- `IPCHANGER-772-LIVE-001 = PASS`
- `CLASSIC-LOGIN-772-001 = PASS`
- `CLASSIC-CHARLIST-772-001 = PASS`
- `CLASSIC-GAME-ENTRY-772-001 = PASS`
- `CLASSIC-SESSION-SUSTAIN-001 = PASS` for the observed interval greater than 30 minutes
- `CLASSIC-CLIENT-772-001 = IN_PROGRESS` because original acquisition provenance is `UNKNOWN` and the live procedure has not been independently repeated

## Scope boundary and remaining work

This evidence proves this selected local artifact set can traverse the classic Login and Game path against the sanitized Fusion32 runtime and remain connected. It does not certify historical provenance, every protocol field, gameplay correctness, packet fixtures, two-client parity or Unreal behavior. No screenshot or packet capture was retained in this run.

Several stale `CLOSE-WAIT` sockets were observed between Game/Login and Query Manager while the three services and client session remained healthy. Their cause and long-run resource impact are `UNKNOWN` and should be investigated separately.
