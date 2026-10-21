$ErrorActionPreference='Stop'
$certRoot=(Get-Location).Path
$dest=Join-Path $certRoot 'build/certification/automation-probe'
New-Item -ItemType Directory -Path $dest -Force | Out-Null
$argsAuto=@(('"'+(Join-Path $certRoot 'unreal/REAL33D/REAL33D.uproject')+'"'),'-unattended','-nop4','-nullrhi','-nosplash','-ExecCmds="Automation RunTests REAL33D.Presentation+REAL33D.WideWorld+REAL33D.Certification"','-TestExit="Automation Test Queue Empty"',('-ReportExportPath="'+$dest+'/report"'),('-abslog="'+$dest+'/automation.log"'))
$p=Start-Process -FilePath 'C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' -ArgumentList $argsAuto -WindowStyle Hidden -PassThru -Wait
if($p.ExitCode -ne 0){exit $p.ExitCode}
$r=Get-Content -Raw -LiteralPath "$dest/report/index.json" | ConvertFrom-Json
$r.tests | Select-Object fullTestPath,state,errors,warnings | ConvertTo-Json
if($r.failed -ne 0 -or $r.notRun -ne 0 -or $r.inProcess -ne 0 -or $r.succeeded -ne 10){throw 'Expected ten passing tests'}
