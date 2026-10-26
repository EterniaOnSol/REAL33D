$ErrorActionPreference = 'Stop'
$certRoot = 'C:\Users\dell\Desktop\fusion32\build\cert-login-ui-independent-20261025'
$outDir = Join-Path $certRoot 'build\certification\automation'
New-Item -ItemType Directory -Force $outDir | Out-Null
if (Test-Path (Join-Path $outDir 'automation-report\index.json')) { throw 'Fresh output required' }
$automationArgs = @(
 ('"' + $certRoot + '\unreal\REAL33D\REAL33D.uproject"'),
 '-unattended', '-nop4', '-nullrhi', '-nosplash',
 '-ExecCmds="Automation RunTests REAL33D"', '-TestExit="Automation Test Queue Empty"',
 ('-ReportExportPath="' + $outDir + '\automation-report"'),
 ('-abslog="' + $outDir + '\automation.log"')
)
$process = Start-Process -FilePath 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' -ArgumentList $automationArgs -WindowStyle Hidden -PassThru -Wait
if ($process.ExitCode -ne 0) { throw ('Unreal exit ' + $process.ExitCode) }
$report = Get-Content (Join-Path $outDir 'automation-report\index.json') -Raw | ConvertFrom-Json
$report.tests | Select-Object fullTestPath,state,errors,warnings | ConvertTo-Json
if ($report.succeeded -ne 14 -or $report.failed -ne 0 -or $report.notRun -ne 0 -or $report.inProcess -ne 0) { throw 'Expected exactly 14 completed passing tests' }
Write-Output 'PASS 14/14'

