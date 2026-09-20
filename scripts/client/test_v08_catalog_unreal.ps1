$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$editor = 'C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
$project = Join-Path $repo 'unreal/REAL33D/REAL33D.uproject'
$catalog = Join-Path $repo 'visual/qa/full_catalog_v08/experimental_catalog_runtime.json'
& $editor $project "-real33d-experimental-catalog=$catalog" `
    '-ExecCmds=Automation RunTests REAL33D.AssetRegistry.ExperimentalV08;Quit' `
    '-TestExit=Automation Test Queue Empty' '-unattended' '-nop4' '-nosplash' `
    '-nullrhi' '-DDC-ForceMemoryCache' '-NoSaveConfig' '-stdout'
if ($LASTEXITCODE -ne 0) { throw "experimental V08 registry test failed with exit code $LASTEXITCODE" }
