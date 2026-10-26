$ErrorActionPreference='Stop'
$certRoot=(Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$outDir=Join-Path $PSScriptRoot 'automation-desktop'
New-Item -ItemType Directory -Force $outDir | Out-Null
if(Test-Path ($outDir+'/report/index.json')){throw 'Fresh automation output required'}
$automationArgs=@(('"'+$certRoot+'/unreal/REAL33D/REAL33D.uproject"'),'-unattended','-nop4','-nullrhi','-nosplash','-ExecCmds="Automation RunTests REAL33D"','-TestExit="Automation Test Queue Empty"',('-ReportExportPath="'+$outDir+'/report"'),('-abslog="'+$outDir+'/automation.log"'))
$proc=Start-Process -FilePath 'C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' -ArgumentList $automationArgs -WindowStyle Hidden -PassThru -Wait
if($proc.ExitCode -ne 0){throw ('Unreal exit '+$proc.ExitCode)}
$report=Get-Content ($outDir+'/report/index.json') -Raw | ConvertFrom-Json
$report.tests | Select-Object fullTestPath,state,errors,warnings | ConvertTo-Json
if($report.succeeded -lt 15 -or $report.failed -ne 0 -or $report.notRun -ne 0 -or $report.inProcess -ne 0){throw 'Expected at least 15 completed passing tests'}
Write-Output ('PASS '+$report.succeeded+'/'+$report.succeeded)
