$ErrorActionPreference='Stop'
$certRoot=(Get-Location).Path
$live=Join-Path $certRoot ('build/certification/live-'+(Get-Date -Format 'yyyyMMddTHHmmss'))
New-Item -ItemType Directory -Path $live -Force | Out-Null
$liveArgs=@(
('"'+(Join-Path $certRoot 'unreal/REAL33D/REAL33D.uproject')+'"'),
'-game','-windowed','-ResX=1600','-ResY=1000','-nosound',
'-real33d-runtime="\\wsl.localhost\Ubuntu-26.04\var\lib\fusion32-server-baseline-772-0"',
'-real33d-account=B','-real33d-host=127.0.0.1','-real33d-loginport=7171',
'-real33d-cert-outfit','-real33d-minimap-qa','-real33d-disable-inspector','-ExecCmds="t.MaxFPS 30"',
('-real33d-evidence="'+$live+'"'),('-abslog="'+(Join-Path $live 'REAL33D.log')+'"'))
$proc=Start-Process -FilePath 'C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe' -ArgumentList $liveArgs -WindowStyle Hidden -PassThru
@{pid=$proc.Id;evidence=$live;candidate='61ceaa955933672c526864b5ed24dca0e4c8de84'} | ConvertTo-Json | Set-Content build/certification/live-session.json
Write-Output ('LIVE_PID='+$proc.Id)
Write-Output ('LIVE_EVIDENCE='+$live)


