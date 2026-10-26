param([string]$Run, [string]$Account = 'B', [switch]$QA)
$ErrorActionPreference = 'Stop'
$certRoot = 'C:\Users\dell\Desktop\fusion32\build\cert-login-ui-independent-20261025'
$outDir = Join-Path $certRoot ('build\certification\live\' + $Run)
if (Test-Path (Join-Path $outDir 'launch.json')) { throw 'Run already exists' }
New-Item -ItemType Directory -Force $outDir | Out-Null
$launchArgs = @(
 ('"' + $certRoot + '\unreal\REAL33D\REAL33D.uproject"'),
 '-game', '-windowed', '-ResX=1280', '-ResY=900', '-nosound', '-nosplash',
 '-ExecCmds="t.MaxFPS 20"',
 '-real33d-runtime="\\wsl.localhost\Ubuntu-26.04\var\lib\fusion32-server-baseline-772-0"',
 ('-real33d-account=' + $Account), '-real33d-host=127.0.0.1', '-real33d-loginport=7171',
 ('-real33d-evidence="' + $outDir + '"'), ('-abslog="' + $outDir + '\REAL33D.log"')
)
if ($QA) { $launchArgs += @('-real33d-login-qa', '-real33d-login-qa-cycles=4') }
$client = Start-Process -FilePath 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe' -ArgumentList $launchArgs -WindowStyle Hidden -PassThru
$data = @{
 pid=$client.Id; account_alias=$Account; utc=[DateTime]::UtcNow.ToString('o'); args=$launchArgs
 implementation='bf54da4c576859fdd7ac7188d7a9f925e966d421'
 dll_sha256=(Get-FileHash (Join-Path $certRoot 'unreal\REAL33D\Binaries\Win64\UnrealEditor-REAL33D.dll') -Algorithm SHA256).Hash
}
$data | ConvertTo-Json -Depth 4 | Set-Content (Join-Path $outDir 'launch.json') -Encoding UTF8
Write-Output ('Launched ' + $Run + ' PID ' + $client.Id)

