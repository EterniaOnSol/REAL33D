# Rebuild the approved obj:3501 Unreal assets from the LFS-tracked GLB.
# Run after the normal ClientCore and REAL33DEditor builds in a clean checkout.
$ErrorActionPreference = 'Stop'
$repo = (Resolve-Path (Join-Path $PSScriptRoot '../..')).Path
$editor = 'C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe'
$project = Join-Path $repo 'unreal/REAL33D/REAL33D.uproject'
$script = (Join-Path $repo 'visual/tools/import_obj3501_unreal.py').Replace('\', '/')
if (-not (Test-Path -LiteralPath $editor)) {
    throw "Unreal Engine 5.8 editor commandlet missing: $editor"
}
& $editor $project '-EnablePlugins=PythonScriptPlugin' '-run=PythonScript' "-script=$script" '-unattended' '-nop4' '-nosplash' '-nullrhi' '-DDC-ForceMemoryCache' '-stdout'
if ($LASTEXITCODE -ne 0) {
    throw "obj:3501 import failed with exit code $LASTEXITCODE"
}
