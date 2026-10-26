$ErrorActionPreference = 'Stop'
$certRoot = 'C:/Users/dell/Desktop/fusion32/build/cert-login-ui-independent-20261025'
$outDir = "$certRoot/build/certification/live/A-art"
if (Test-Path "$outDir/launch.json") { throw 'Run already exists' }
New-Item -ItemType Directory -Force $outDir | Out-Null
$launchArgs = @(
 ('"' + $certRoot + '/unreal/REAL33D/REAL33D.uproject"'),
 '-game', '-windowed', '-ResX=1280', '-ResY=900', '-nosound', '-nosplash',
 '-ExecCmds="t.MaxFPS 20"',
 '-real33d-runtime="\\wsl.localhost\Ubuntu-26.04\var\lib\fusion32-server-baseline-772-0"',
 '-real33d-account=A', '-real33d-host=127.0.0.1', '-real33d-loginport=7171',
 ('-real33d-evidence="' + $outDir + '"'), ('-abslog="' + $outDir + '/REAL33D.log"'),
 ('-real33d-creature-catalog="' + $certRoot + '/visual/qa/brother_creatures/runtime.json"'),
 ('-real33d-experimental-catalog="' + $certRoot + '/visual/qa/full_catalog_v08/experimental_catalog_runtime.json"'),
 '-real33d-disable-inspector', '-real33d-presentation-qa'
)
$client = Start-Process -FilePath 'C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe' -ArgumentList $launchArgs -PassThru
@{
 pid=$client.Id; account_alias='A'; utc=[DateTime]::UtcNow.ToString('o'); args=$launchArgs
 implementation='bf54da4c576859fdd7ac7188d7a9f925e966d421'
 dll_sha256=(Get-FileHash "$certRoot/unreal/REAL33D/Binaries/Win64/UnrealEditor-REAL33D.dll").Hash
 purpose='User requested existing artwork and parking near Cipfried; separate from completed same-process QA cycles.'
} | ConvertTo-Json -Depth 4 | Set-Content "$outDir/launch.json" -Encoding UTF8
Write-Output ('Launched A-art PID '+$client.Id)
