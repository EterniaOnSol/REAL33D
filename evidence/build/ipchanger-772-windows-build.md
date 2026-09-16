# Fusion32 IP Changer 7.72 Build Evidence

Date: 2026-09-15

Source artifact: `ipchanger.bundle` from `tibiacacaca.zip`
Bundle SHA-256: `2CC5415809D036B7C9191433EA091F84ED242EAF796751B289F3084C8D48E044`
Revision: `8215db18abbae05b62bcbd5c4f086856168283a4`
Tree: `aa78645284e1da20384dd9789840415cfa296234`

## Source identity

`git bundle verify` reported a complete bundle with HEAD/master at the selected revision. All six materialized upstream repository blobs matched the selected tree. The Game README identifies this as the IP Changer authored by Fusion32. No archived executable was extracted or run.

## Build

Toolchain: Visual Studio Build Tools 2022 developer prompt v17.14.40, x86 target.
Command: `tests\build_ipchanger_windows.cmd` after `VsDevCmd.bat -arch=x86 -host_arch=x64`.
Result: exit 0 with `/W3 /WX`; `ipchanger.cc` and `memscan.cc` compiled.

| Output | PE machine | SHA-256 |
| --- | --- | --- |
| `ipchanger.exe` | `14C` / x86 | `39053781B44850901FA8B8F679BC2D971EC1CFE3833F4FACCB065876E0A0201B` |
| `memscan.exe` | `14C` / x86 | `6BEF001CC60F2BF4FB9A3842BD75667D6FFC33551D919DF8FE72630930E71EBB` |

Build products remain under ignored `build/ipchanger/`. Neither executable was run.

## Deterministic configuration check

The sanitized runtime generated equivalent hexadecimal and decimal public moduli. `prepare_fusion32_ipchanger_wsl.sh` produced an ignored five-field entry with version 772, alias `fusion32`, host `127.0.0.1`, port `7171`, and a 309-character decimal modulus. The modulus value was not captured. `runtime_smoke_wsl.sh` proved the decimal value equals the generated hexadecimal public modulus and fits the source-defined 312-byte field including its terminator.

Status: `BUILD PASS` and `CONFIG GENERATION PASS`. Live client-memory patching remains `BLOCKED_CLIENT_ABSENT`; this evidence is not a classic login or protocol PASS.

`IPCHANGER-772-REVIEW-001 = ACCEPT`: an independent reviewer re-extracted and verified the bundle, matched all six blobs and the complete 7.72 address table, checked decimal-modulus semantics and limits, corroborated both PE x86 output hashes, and confirmed that no binary was executed. Live compatibility was explicitly excluded.
