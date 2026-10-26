param([Parameter(Mandatory=$true)][string]$Run,[string]$Account='B',[switch]$QA,[switch]$Quit,[int]$Cycles=4)
$ErrorActionPreference='Stop'
$certRoot=(Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$outDir=Join-Path $PSScriptRoot ('live/'+$Run)
if(Test-Path $outDir){throw 'Fresh run name required'}
New-Item -ItemType Directory -Path $outDir | Out-Null
$launchArgs=@(('"'+$certRoot+'/unreal/REAL33D/REAL33D.uproject"'),'-game','-windowed','-ResX=1280','-ResY=900','-nosound','-nosplash','-ExecCmds="t.MaxFPS 20"','-real33d-runtime=\\wsl.localhost\Ubuntu-26.04\var\lib\fusion32-server-baseline-772-0',('-real33d-account='+$Account),'-real33d-host=127.0.0.1','-real33d-loginport=7171',('-real33d-evidence="'+$outDir+'"'),('-abslog="'+$outDir+'/REAL33D.log"'))
if($QA){$launchArgs+=@('-real33d-login-qa',('-real33d-login-qa-cycles='+$Cycles))}
if($Quit){$launchArgs+='-real33d-login-qa-quit'}
$client=Start-Process -FilePath 'C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor.exe' -ArgumentList $launchArgs -WindowStyle Hidden -PassThru
@{pid=$client.Id;process_start_utc=$client.StartTime.ToUniversalTime().ToString('o');utc=[DateTime]::UtcNow.ToString('o');args=$launchArgs;head=(git -C $certRoot rev-parse HEAD);dll_sha256=(Get-FileHash ($certRoot+'/unreal/REAL33D/Binaries/Win64/UnrealEditor-REAL33D.dll')).Hash} | ConvertTo-Json -Depth 4 | Set-Content ($outDir+'/launch.json') -Encoding UTF8
Write-Output ('Launched '+$Run+' PID '+$client.Id)
