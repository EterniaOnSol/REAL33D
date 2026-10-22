$ErrorActionPreference = 'Stop'
$certRoot = 'C:\Users\dell\Desktop\fusion32\build\cert-diagonal-input-20261022'
if (Get-Process UnrealEditor -ErrorAction SilentlyContinue) { throw 'An Unreal editor session already exists' }
$outDir = Join-Path $certRoot 'build\certification\live'
New-Item -ItemType Directory -Path $outDir -Force | Out-Null
$argsList = @(
  ('"' + $certRoot + '\unreal\REAL33D\REAL33D.uproject"'),
  '-game', '-windowed', '-ResX=1600', '-ResY=900', '-nosound', '-nosplash',
  '-real33d-runtime="\\wsl.localhost\Ubuntu-26.04\var\lib\fusion32-server-baseline-772-0"',
  '-real33d-account=B', '-real33d-host=127.0.0.1', '-real33d-loginport=7171',
  '-ExecCmds="t.MaxFPS 30"',
  ('-real33d-evidence="' + $outDir + '"'),
  ('-abslog="' + $outDir + '\REAL33D.log"')
)
$proc = Start-Process -FilePath 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe' -ArgumentList $argsList -PassThru
$dll = Get-FileHash -Algorithm SHA256 (Join-Path $certRoot 'unreal\REAL33D\Binaries\Win64\UnrealEditor-REAL33D.dll')
@{ pid=$proc.Id; arguments=$argsList; project=$certRoot; utc=[DateTime]::UtcNow.ToString('o'); dll_sha256=$dll.Hash; implementation='eb17e26ed79f63bf60e6ec709b9c1540ab008694' } | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $outDir 'launch.json')
Write-Output "LIVE_PID=$($proc.Id)"
