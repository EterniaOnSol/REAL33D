$ErrorActionPreference = 'Stop'
$root = (Resolve-Path '.').Path
if ($root -ne 'C:\Users\dell\Desktop\fusion32\build\cert-stackable-20261015') { throw 'Wrong checkout' }
if (Get-Process -Id 3600 -ErrorAction SilentlyContinue) { throw 'Close live QA before building' }
$test = Join-Path $root 'unreal/REAL33D/Source/REAL33D/Private/StackSpriteCertification.cpp'
if (Test-Path -LiteralPath $test) {
    if ((Get-FileHash -LiteralPath $test).Hash -ne (Get-FileHash -LiteralPath 'build/certification/StackSpriteCertification.cpp').Hash) { throw 'Unexpected diagnostic source' }
} else { Copy-Item -LiteralPath 'build/certification/StackSpriteCertification.cpp' -Destination $test }
$project = Join-Path $root 'unreal/REAL33D/REAL33D.uproject'
$ue = 'C:\Program Files\Epic Games\UE_5.8\Engine'
$evidence = Join-Path $root 'evidence/clientcore/unreal-stackable-sprite-independent-20261015'
& "$ue/Build/BatchFiles/Build.bat" REAL33DEditor Win64 Development "-Project=$project" -WaitMutex -NoUBTMakefiles 2>&1 | Tee-Object -FilePath "$evidence/slate-test-build.txt"
if ($LASTEXITCODE -ne 0) { throw 'Diagnostic test build failed' }
& "$ue/Binaries/Win64/UnrealEditor-Cmd.exe" $project -unattended -nop4 -nosplash -NullRHI '-ExecCmds=Automation RunTests REAL33D.Certification.StackableSpriteFiles' '-TestExit=Automation Test Queue Empty' "-ReportExportPath=$root/build/certification/slate-report" "-abslog=$root/build/certification/slate-automation.log" -stdout -FullStdOutLogOutput > "$root/build/certification/slate-stdout.txt" 2>&1
if ($LASTEXITCODE -ne 0) { throw 'Diagnostic automation failed; inspect report' }
$report = Get-Content -LiteralPath "$root/build/certification/slate-report/index.json" -Raw | ConvertFrom-Json
$result = @($report.tests | Where-Object fullTestPath -eq 'REAL33D.Certification.StackableSpriteFiles')
if ($result.Count -ne 1 -or $result[0].state -ne 'Success' -or $result[0].errors -ne 0 -or $report.failed -ne 0 -or $report.notRun -ne 0 -or $report.inProcess -ne 0) { throw 'Required diagnostic test did not pass' }
Write-Output 'PASS: independent Slate file fallback and generic resource selection'
