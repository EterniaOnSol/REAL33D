$ErrorActionPreference = 'Stop'
$live = Join-Path $PSScriptRoot 'live'
$log = Get-Content -LiteralPath (Join-Path $live 'REAL33D.log') -Raw
if ($log -notmatch 'CHANNELS_CERT_RESULT=PASS step=15' -or $log -match 'CERT_ASSERT FAILED') {
    throw 'The independent live driver did not complete successfully.'
}
$passes = [regex]::Matches($log, 'CERT_ASSERT PASS').Count
if ($passes -ne 58) { throw "Expected 58 live assertions, found $passes" }
foreach ($step in 0..14) {
    if ($log -notmatch ('CERT_STAGE_COMPLETE ' + $step + '\r?\n')) { throw "Missing stage $step" }
}
$snapshots = @(Get-ChildItem -LiteralPath $live -Filter '*.json')
if ($snapshots.Count -ne 9) { throw 'Expected all nine original snapshots.' }
foreach ($snapshot in $snapshots) {
    $data = Get-Content -LiteralPath $snapshot.FullName -Raw | ConvertFrom-Json
    foreach ($counter in @('residual_bytes', 'unsupported_opcodes', 'protocol_anomalies')) {
        if ($null -eq $data.clientcore.$counter -or $data.clientcore.$counter -ne 0) {
            throw ($snapshot.Name + ': ' + $counter + ' is missing or nonzero')
        }
    }
}
foreach ($who in @('A_FINAL', 'B_FINAL')) {
    if ($log -notmatch ('CERT_COUNTERS ' + $who + ' frames=\d+ commands=\d+ residual=0 unsupported=0 anomalies=0 lists=1/1 joins=2 opens=2 leaves=2 channel_send=\d+ channel_receive=\d+ private_send=1 private_receive=1')) {
        throw "Missing successful final wire/arrival counters for $who"
    }
}
$automation = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'automation/index.json') -Raw | ConvertFrom-Json
$expected = @('REAL33D.Presentation.CameraAndFloors', 'REAL33D.Presentation.PanelRegistry',
    'REAL33D.Presentation.SkillsPanel', 'REAL33D.Presentation.ChannelsPanel',
    'REAL33D.Presentation.ChatTabs', 'REAL33D.WideWorld.AuthoritativeWindow')
if (@($automation.tests).Count -ne 6 -or $automation.failed -ne 0 -or $automation.notRun -ne 0 -or $automation.inProcess -ne 0) {
    throw 'Unreal automation is incomplete or failed.'
}
foreach ($name in $expected) {
    $test = @($automation.tests | Where-Object { $_.fullTestPath -eq $name })
    if ($test.Count -ne 1 -or $test[0].state -ne 'Success' -or $test[0].errors -ne 0 -or $test[0].warnings -ne 0) {
        throw "Unreal test did not pass cleanly: $name"
    }
}
$harness = @(Get-ChildItem -LiteralPath (Join-Path $PSScriptRoot 'harness') -Filter 'REGRESSION-HARNESS-*.md')
if ($harness.Count -ne 1 -or (Get-Content -LiteralPath $harness[0].FullName -Raw) -notmatch '9 PASS, 0 FAIL, 0 SKIP') {
    throw 'Corrected strict harness did not pass all nine gates.'
}
$core = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'harness/logs/clientcore-tests.txt') -Raw
if ($core -notmatch '100% tests passed, 0 tests failed out of 13') { throw 'ClientCore 13/13 missing.' }
$native = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'harness/logs/real33d-native-tests.txt') -Raw
if ([regex]::Matches($native, '=== real33d_\w+_tests ===').Count -ne 5 -or $native -notmatch 'runtime MultiUse metadata PASS: 204 types') {
    throw 'Five native tests with runtime metadata were not all executed.'
}
$selftest = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'selftest.txt') -Raw
if ($selftest -notmatch '10 passed, 0 failed') { throw 'Harness selftest 10/10 missing.' }
$secret = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'harness/logs/secret-check.txt') -Raw
if ([regex]::Matches($secret, 'ok\s+:').Count -ne 6 -or $secret -notmatch 'secret_check: PASS') { throw 'Secret check 6/6 missing.' }
Write-Output 'PASS: live 58/58; stages 15/15; protocol snapshots 9/9 zero; both peers final counters; exact-source Unreal automation 6/6; ClientCore 13/13; native 5/5; harness 9/0/0; selftest 10/10; secret_check 6/6.'
