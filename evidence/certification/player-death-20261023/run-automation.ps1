$ErrorActionPreference = 'Stop'
$certRoot = 'C:\Users\dell\Desktop\fusion32\build\cert-player-death-20261023'
$outDir = Join-Path $certRoot 'build\certification\automation'
New-Item -ItemType Directory -Path $outDir -Force | Out-Null
$argsList = @(
  ('"' + $certRoot + '\unreal\REAL33D\REAL33D.uproject"'),
  '-unattended', '-nop4', '-nullrhi', '-nosplash',
  '-ExecCmds="Automation RunTests REAL33D.Presentation+REAL33D.WideWorld"',
  '-TestExit="Automation Test Queue Empty"',
  ('-ReportExportPath="' + $outDir + '\automation-report"'),
  ('-abslog="' + $outDir + '\automation.log"')
)
$proc = Start-Process -FilePath 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' -ArgumentList $argsList -WindowStyle Hidden -PassThru
$proc.WaitForExit()
if ($proc.ExitCode -ne 0) { throw "Unreal exit $($proc.ExitCode)" }
$report = Get-Content -LiteralPath (Join-Path $outDir 'automation-report\index.json') -Raw | ConvertFrom-Json
$report.tests | Select-Object fullTestPath,state,errors
if ($report.succeeded -lt 10 -or $report.failed -ne 0 -or $report.notRun -ne 0 -or $report.inProcess -ne 0) { throw 'Incomplete or failing automation suite' }
Write-Output "PASS $($report.succeeded) tests"
