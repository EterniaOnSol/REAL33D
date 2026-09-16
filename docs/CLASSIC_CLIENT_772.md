# Classic Tibia 7.72 Client Gate

Status: `IN_PROGRESS`. Static compatibility, live IP Changer patching, Login authentication, character list, Game entry and a sustained session are `PASS` for the selected local artifact set. Historical acquisition provenance is `UNKNOWN`; independent live repetition is pending.

| Requirement | Current value |
| --- | --- |
| Version | Tibia 7.72 desktop client |
| Selected executable | ignored local `build/classic-client-772/app/Tibia.exe` |
| Executable SHA-256 | `1A82FE97A39E536C29327D6D5DDE316775F537DCC01506831429B1A0433B76F8` |
| Companion data | exact DAT/SPR/PIC hashes recorded in `evidence/client/CLASSIC-CLIENT-772-001.md` |
| Provenance | `USER-SUPPLIED_LOCAL_COPY`; pre-existing on the operator's computer; original source/date/chain of custody `UNKNOWN` |
| Login host | `127.0.0.1` for local WSL testing |
| Login port | `7171` |
| Game endpoint | returned by Login from world row: `127.0.0.1:7172` |
| RSA public modulus | fresh runtime key; decimal form generated outside Git for the Fusion32 IP Changer |
| IP Changer | Fusion32 revision `8215db18...`; Windows x86 build, configuration and live patch `PASS` |

PE resources identify the file as `Tibia Player` version `7.72`, company `CipSoft GmbH`. No `Zone.Identifier`, original archive, installer or download record was present. Those facts support identity but do not establish historical provenance or licensing. Never invent a source for this copy.

Because the sanitized server uses a fresh key, the client process must receive its matching public modulus. Only the locally built Fusion32-authored IP Changer was used. The historical private key and archived binaries remain unused.

## Fusion32 7.72 address table

| Field | Source-defined value | Selected client result |
| --- | --- | --- |
| Version marker | `Version 7.72` at `0x55C65D` | static `PASS` |
| Login endpoints | 5 entries, stride 112 bytes | static range and live patch `PASS` |
| First hostname | `0x7152F8`; maximum 100 bytes including terminator | live local Login reachability `PASS` |
| First port | `0x71535C`; four little-endian bytes | live `7171` reachability `PASS` |
| RSA modulus | `0x55B620`; maximum 312 bytes including terminator | live RSA/Login flow `PASS` |

The tool finds a window of class `TibiaClient`, validates the version marker, patches all five Login endpoints, temporarily makes the read-only RSA area writable, writes the decimal modulus and restores protection. Successful traversal through Login to an established Game connection validates those source-defined writes for this exact process image without retaining process memory or key material.

## Reproducible preparation and static verification

From a Visual Studio x86 developer environment:

```bat
tests\build_ipchanger_windows.cmd
```

Generate the ignored runtime-specific `servers.txt` without printing the modulus:

```powershell
wsl.exe -d Ubuntu-26.04 -- bash /mnt/c/Users/dell/Desktop/fusion32/scripts/client/prepare_fusion32_ipchanger_wsl.sh /mnt/c/Users/dell/Desktop/fusion32
```

Verify the exact local executable and DAT/SPR/PIC set without executing them:

```powershell
wsl.exe -d Ubuntu-26.04 -- python3 /mnt/c/Users/dell/Desktop/fusion32/tests/verify_classic_client_772.py /mnt/c/Users/dell/Desktop/fusion32/build/classic-client-772/app/Tibia.exe
```

The generated IP Changer entry is `772;fusion32;127.0.0.1;7171;<decimal modulus>`. A runtime reset changes credentials and the RSA modulus, so regenerate this file after every reset. Never commit the client artifacts, `servers.txt`, passwords, private key or modulus.

## Current live results

- `CLASSIC-CLIENT-772-STATIC-001 = PASS`
- `IPCHANGER-772-LIVE-001 = PASS`
- `CLASSIC-LOGIN-772-001 = PASS`
- `CLASSIC-CHARLIST-772-001 = PASS`
- `CLASSIC-GAME-ENTRY-772-001 = PASS`
- `CLASSIC-SESSION-SUSTAIN-001 = PASS` for the observed interval greater than 30 minutes
- `CLASSIC-CLIENT-772-001 = IN_PROGRESS`

Full preconditions, hashes, observations and scope limits are in `evidence/client/CLASSIC-CLIENT-772-001.md`.

## Certification boundary

An independent reviewer must repeat the clean runtime, regenerated modulus, static verification, live patch, character-list selection, world entry and sustained-session checks before functional compatibility can become `CERTIFIED`. Preserve sanitized screenshots of the character list and initial world in that repetition.

Historical provenance cannot become certified from this copy alone because its original source and acquisition record are absent. Either obtain an authorized client artifact with verifiable provenance or explicitly certify only functional compatibility of the recorded hashes while retaining provenance as `UNKNOWN`.
