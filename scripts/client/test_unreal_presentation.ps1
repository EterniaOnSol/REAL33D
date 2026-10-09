param([string]$EvidenceDirectory = '', [string]$ExperimentalCatalog = '')
$ErrorActionPreference = 'Stop'
$polishRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$polishEvidence = if ($EvidenceDirectory) { [IO.Path]::GetFullPath($EvidenceDirectory) } else {
 Join-Path $polishRoot ('build\unreal-world-presentation-polish-001\checks-' + (Get-Date -Format 'yyyyMMddTHHmmss'))
}
New-Item -ItemType Directory -Path $polishEvidence -Force | Out-Null
$polishReport = Join-Path $polishEvidence 'automation-report\index.json'
if (Test-Path -LiteralPath $polishReport) { throw 'Use a fresh evidence directory; an automation report already exists.' }
$polishExpected = @('REAL33D.Presentation.CameraAndFloors', 'REAL33D.WideWorld.AuthoritativeWindow')
$polishTests = 'REAL33D.Presentation+REAL33D.WideWorld'
if ($ExperimentalCatalog) {
 if (!(Test-Path -LiteralPath $ExperimentalCatalog)) { throw "Missing catalog: $ExperimentalCatalog" }
 $polishTests += '+REAL33D.AssetRegistry.ExperimentalV08'
 $polishExpected += 'REAL33D.AssetRegistry.ExperimentalV08'
}
$polishArgs = @(
 ('"' + (Join-Path $polishRoot 'unreal\REAL33D\REAL33D.uproject') + '"'),
 '-unattended', '-nop4', '-nullrhi', '-nosplash',
 ('-ExecCmds="Automation RunTests ' + $polishTests + '"'),
 '-TestExit="Automation Test Queue Empty"',
 ('-ReportExportPath="' + (Join-Path $polishEvidence 'automation-report') + '"'),
 ('-abslog="' + (Join-Path $polishEvidence 'automation-tests.log') + '"')
)
if ($ExperimentalCatalog) { $polishArgs += ('-real33d-experimental-catalog="' + [IO.Path]::GetFullPath($ExperimentalCatalog) + '"') }
$polishProcess = Start-Process -FilePath 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' -ArgumentList $polishArgs -WindowStyle Hidden -PassThru -Wait
if ($polishProcess.ExitCode -ne 0) { exit $polishProcess.ExitCode }
if (!(Test-Path -LiteralPath $polishReport)) { throw 'Unreal exited without an automation report.' }
$polishResults = Get-Content -LiteralPath $polishReport -Raw | ConvertFrom-Json
foreach ($polishName in $polishExpected) {
 $polishResult = @($polishResults.tests | Where-Object { $_.fullTestPath -eq $polishName })
 if ($polishResult.Count -ne 1 -or $polishResult[0].state -ne 'Success' -or $polishResult[0].errors -ne 0) {
  throw "Required automation test did not pass: $polishName"
 }
}
if ($polishResults.failed -ne 0 -or $polishResults.notRun -ne 0 -or $polishResults.inProcess -ne 0) {
 throw 'Automation report contains failed, skipped, or unfinished tests.'
}
Write-Output ('PASS: ' + ($polishExpected -join ', '))
exit 0
