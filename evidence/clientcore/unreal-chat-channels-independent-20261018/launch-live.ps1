$ErrorActionPreference = 'Stop'
$certRoot = (Get-Location).Path
$live = Join-Path $certRoot ('build/certification/live-' + (Get-Date -Format 'yyyyMMddTHHmmss'))
New-Item -ItemType Directory -Path $live -Force | Out-Null
$argsLive = @(
 ('"' + (Join-Path $certRoot 'unreal/REAL33D/REAL33D.uproject') + '"'),
 '-game', '-windowed', '-ResX=1600', '-ResY=900', '-nosound',
 '-real33d-runtime="\\wsl.localhost\Ubuntu-26.04\var\lib\fusion32-server-baseline-772-0"',
 '-real33d-account=B', '-real33d-host=127.0.0.1', '-real33d-loginport=7171',
 '-real33d-minimap-qa', '-real33d-cert-channels', '-real33d-disable-inspector',
 '-ExecCmds="t.MaxFPS 30"',
 ('-real33d-evidence="' + $live + '"'), ('-abslog="' + (Join-Path $live 'REAL33D.log') + '"')
)
$p = Start-Process -FilePath 'C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe' -ArgumentList $argsLive -WindowStyle Hidden -PassThru
@{pid=$p.Id;evidence=$live;candidate='239f89f07430ecd4d4348d69f183a9935e4bd992'} | ConvertTo-Json | Set-Content build/certification/live-session.json
Write-Output ('LIVE_PID=' + $p.Id)
Write-Output ('LIVE_EVIDENCE=' + $live)
