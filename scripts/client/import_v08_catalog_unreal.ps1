param(
    [string]$Source = 'C:\Users\dell\3DTIBIA_leo',
    [int]$Start = 0,
    [int]$Limit = 0,
    [switch]$RetryFailed
)

$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$editor = 'C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
$project = Join-Path $repo 'unreal/REAL33D/REAL33D.uproject'
$script = (Join-Path $repo 'visual/tools/import_v08_catalog_unreal.py').Replace('\', '/')
$manifest = (Join-Path $repo 'visual/qa/full_catalog_v08/full_catalog_manifest.json').Replace('\', '/')
if (-not (Test-Path -LiteralPath $editor)) { throw "Unreal Engine 5.8 missing: $editor" }
if (-not (Test-Path -LiteralPath $Source)) { throw "3DTIBIA source missing: $Source" }
if (-not (Test-Path -LiteralPath $manifest)) { throw "Build the normalized V08 manifest first: $manifest" }
$retry = if ($RetryFailed) { '1' } else { '0' }
& $editor $project '-EnablePlugins=PythonScriptPlugin' '-run=PythonScript' "-script=$script" `
    "-fullcatalog-source=$Source" "-fullcatalog-manifest=$manifest" `
    "-fullcatalog-start=$Start" "-fullcatalog-limit=$Limit" `
    "-fullcatalog-retry-failed=$retry" '-unattended' '-nop4' '-nosplash' `
    '-nullrhi' '-DDC-ForceMemoryCache' '-NoSaveConfig' '-stdout'
if ($LASTEXITCODE -ne 0) { throw "V08 import batch failed with exit code $LASTEXITCODE" }
