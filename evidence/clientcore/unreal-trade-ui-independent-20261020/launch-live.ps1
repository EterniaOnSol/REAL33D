$ErrorActionPreference = 'Stop'
$certRoot = (Get-Location).Path
$live = Join-Path $certRoot ('build/certification/live-' + (Get-Date -Format 'yyyyMMddTHHmmss'))
New-Item -ItemType Directory -Path $live -Force | Out-Null
$argsLive = @(
 ('"' + (Join-Path $certRoot 'unreal/REAL33D/REAL33D.uproject') + '"'),
 '-game', '-windowed', '-ResX=1800', '-ResY=1050', '-nosound',
 '-real33d-runtime="\\wsl.localhost\Ubuntu-26.04\var\lib\fusion32-server-baseline-772-0"',
 '-real33d-account=B', '-real33d-host=127.0.0.1', '-real33d-loginport=7171',
 '-real33d-minimap-qa', '-real33d-cert-trade', '-real33d-cert-trade-resume', '-real33d-disable-inspector',
 '-ExecCmds="t.MaxFPS 30"',
 ('-real33d-evidence="' + $live + '"'), ('-abslog="' + (Join-Path $live 'REAL33D.log') + '"')
)
$p = Start-Process -FilePath 'C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe' -ArgumentList $argsLive -WindowStyle Hidden -PassThru
@{pid=$p.Id;evidence=$live;candidate='1cf1ec4682eb17df6df8e576da1c822a915e63f4'} | ConvertTo-Json | Set-Content build/certification/live-session.json
Write-Output ('LIVE_PID=' + $p.Id)
Write-Output ('LIVE_EVIDENCE=' + $live)
