# Reproducing this fix's checks

Runtime credentials stay outside Git. `$RT` below is the sanitized runtime root
(`\\wsl.localhost\Ubuntu-26.04\var\lib\fusion32-server-baseline-772-0` on the
host this ran on); nothing here prints a value out of it.

## 1. The unit rule, with no engine and no server

```bash
wsl -d Ubuntu-26.04 -- g++ -std=c++17 -Wall -Wextra -Werror \
  -I unreal/REAL33D/Source/REAL33D/Public \
  -o /tmp/credlog tests/real33d_credential_log_tests.cpp && /tmp/credlog
```

## 2. The module and its automation suite

```powershell
# x64 Native Tools prompt, or call vcvars64.bat first.
& "$env:UE\Engine\Build\BatchFiles\Build.bat" REAL33DEditor Win64 Development `
    -Project="$PWD\unreal\REAL33D\REAL33D.uproject" -WaitMutex

& "$env:UE\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" `
    "$PWD\unreal\REAL33D\REAL33D.uproject" -unattended -nop4 -nullrhi -nosplash `
    -ExecCmds="Automation RunTests REAL33D" -TestExit="Automation Test Queue Empty" `
    -ReportExportPath="$OUT\automation-report" -abslog="$OUT\automation.log"
```

`tests/automation-report-index.json` is that report: 15 of 15 succeeded, 0
failed, 0 notRun, 0 inProcess, and `errors`/`warnings` zero on every test,
including the new `REAL33D.Presentation.CredentialLog`.

## 3. The live login run that produced `live/`

Fusion32 must already be listening on 7171/7172
(`scripts/server/start_wsl.sh`). The driver is the implementation's own opt-in
one; no second path into the bridge exists.

```powershell
& "$env:UE\Engine\Binaries\Win64\UnrealEditor.exe" `
    "$PWD\unreal\REAL33D\REAL33D.uproject" -game -windowed -ResX=1280 -ResY=900 `
    -nosound -nosplash -ExecCmds="t.MaxFPS 20" `
    -real33d-runtime=$RT -real33d-account=B `
    -real33d-host=127.0.0.1 -real33d-loginport=7171 `
    -real33d-evidence="$OUT" -abslog="$OUT\REAL33D.log" `
    -real33d-login-qa -real33d-login-qa-cycles=4 -real33d-login-qa-quit
```

`-real33d-runtime` takes the UNC path WITHOUT surrounding quotes. Quoting it
lets Windows argv collapse `\\wsl.localhost` to `\wsl.localhost`, the client
then cannot read the account file, and the driver stops at
`login QA: the runtime account file is not readable` having typed nothing.

## 4. The strict harness, including the new scan

```bash
tests/regression_harness.sh --strict --unreal \
  --runtime "$RT" \
  --objects-srv build/harness-inputs/objects.srv \
  --logs build/credlog-live-20261026
```

Run it from a shell where MSYS path conversion is ON. With
`MSYS_NO_PATHCONV=1` or `MSYS2_ARG_CONV_EXCL='*'` in the environment, every
check that calls a Windows `python.exe` receives `/c/...` unconverted and fails
for that reason alone; one run here reported `real33d-native-tests`,
`outfit-color-mapping`, `outfit-palette` and `credential-value-scan` as FAIL
purely from that, and all four passed once the variables were dropped.

`--objects-srv` wants a path the WSL converter can map, so the runtime's
`objects.srv` is copied under `build/` first rather than passed as a
`\\wsl.localhost` UNC path.

## 5. The credential scan on its own, and the redaction

```bash
# the check, over any produced logs or evidence
tests/scan_credential_values.py --runtime "$RT" evidence build/<run>

# how the already-committed artifacts in this repository were sanitized
tests/scan_credential_values.py --runtime "$RT" --redact \
  evidence/clientcore/DUAL-CLIENT-LIVE-CAPTURE-001-session1.log \
  evidence/clientcore/DUAL-CLIENT-LIVE-CAPTURE-001-session3.log \
  evidence/clientcore/unreal-login-character-select-ui-001
```

Both modes were checked against planted copies of the real values before being
trusted: the scan returned exit 1 naming `ACCOUNT_B_ID` and
`ACCOUNT_B_PASSWORD` one occurrence each, and `--redact` turned the same two
files into `[REDACTED_ACCOUNT_B_ID]` / `[REDACTED_ACCOUNT_B_PASSWORD]` and
re-scanned them clean at exit 0.
