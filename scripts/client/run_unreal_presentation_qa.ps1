param(
    [string]$Account = 'B',
    [string]$Runtime = '\\wsl.localhost\Ubuntu-26.04\var\lib\fusion32-server-baseline-772-0',
    [string]$AssetWorktree = '',
    [string]$StaticDataRoot = 'C:\Users\dell\Desktop\fusion32',
    [string]$ClassicDat = '',
    [string]$PythonLauncher = 'C:\Users\dell\AppData\Local\Programs\Python\Launcher\py.exe'
)
$ErrorActionPreference = 'Stop'
$taskRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
if (!$AssetWorktree) { $AssetWorktree = $taskRoot }
$editor = 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe'
$project = Join-Path $taskRoot 'unreal\REAL33D\REAL33D.uproject'
$cache = Join-Path $StaticDataRoot 'unreal\REAL33D\Saved\WideWorldCache'
$catalog = Join-Path $AssetWorktree 'visual\qa\full_catalog_v08\experimental_catalog_runtime.json'
$previews = Join-Path $StaticDataRoot 'visual\reference_pack\previews\item'
$creatures = Join-Path $taskRoot 'visual\qa\brother_creatures\runtime.json'
$evidence = Join-Path $taskRoot ('build\unreal-world-presentation-polish-001\live-' + (Get-Date -Format 'yyyyMMddTHHmmss'))
if (!$ClassicDat) { $ClassicDat = Join-Path $StaticDataRoot 'build\classic-client-772\app\Tibia.dat' }
foreach ($required in @($editor, $project, $catalog, $cache, $ClassicDat, $PythonLauncher, (Join-Path $Runtime 'secrets\credentials.env'))) {
    if (!(Test-Path -LiteralPath $required)) { throw "Missing QA prerequisite: $required" }
}
New-Item -ItemType Directory -Path $evidence -Force | Out-Null
& $PythonLauncher -3 (Join-Path $taskRoot 'scripts\client\extract_minimap_palette.py') $ClassicDat (Join-Path $taskRoot 'unreal\REAL33D\Saved\Minimap\appearance-palette.txt') *> (Join-Path $evidence 'palette-generation.txt')
if ($LASTEXITCODE -ne 0) { throw 'Validated minimap palette generation failed; see palette-generation.txt' }
$arguments = @(
    ('"' + $project + '"'), '-game', '-windowed', '-ResX=1280', '-ResY=720',
    ('-real33d-runtime="' + $Runtime + '"'), ('-real33d-account=' + $Account),
    '-real33d-host=127.0.0.1', '-real33d-loginport=7171',
    ('-real33d-experimental-catalog="' + $catalog + '"'),
    ('-real33d-wide-world-cache="' + $cache + '"'),
    ('-real33d-wide-world-previews="' + $previews + '"'),
    ('-real33d-creature-catalog="' + $creatures + '"'), '-real33d-visual-radius=64',
    ('-real33d-evidence="' + $evidence + '"'), '-real33d-minimap-qa', '-real33d-disable-inspector',
    ('-abslog="' + (Join-Path $evidence 'REAL33D.log') + '"')
)
$process = Start-Process -FilePath $editor -ArgumentList $arguments -WindowStyle Hidden -PassThru
Write-Output ('QA_PID=' + $process.Id)
Write-Output ('QA_EVIDENCE=' + $evidence)
Write-Output 'Presentation QA: normal gameplay only. Arrow keys use Tibia cardinal axes. Minimap drag pans; +/- zoom; Up/Dn browse; Home recenters. F9 captures JSON and UI screenshot. Close normally for EndPlay.'
