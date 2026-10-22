param([ValidateRange(1,4)][int]$StartAt = 1)
$ErrorActionPreference = 'Stop'
$certRoot = (Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
$pythonExe = 'C:\Users\dell\AppData\Local\Programs\Python\Python311\python.exe'
Set-Location -LiteralPath $certRoot
$previousName = if ($StartAt -eq 1) { 'C0' } else { 'H' + ($StartAt - 1) }
foreach ($index in $StartAt..4) {
    $newName = 'H' + $index
    & $pythonExe (Join-Path $PSScriptRoot 'live_session.py') --name $newName --previous $previousName
    if ($LASTEXITCODE -ne 0) { throw "Live checks failed for $newName; preserve both clients" }
    $oldMeta = Get-Content -LiteralPath "build/certification/live/$previousName/launch.json" -Raw | ConvertFrom-Json
    $oldProcessId = [int]$oldMeta.pid
    $oldInfo = Get-CimInstance Win32_Process -Filter "ProcessId=$oldProcessId"
    if ($oldInfo.Name -ne 'UnrealEditor.exe' -or $oldInfo.CommandLine -notlike "*$certRoot*" -or $oldInfo.CommandLine -notlike "*live\$previousName*") {
        throw 'Old client process identity mismatch'
    }
    $oldProcess = Get-Process -Id $oldProcessId
    if (-not $oldProcess.CloseMainWindow()) { throw 'Could not close displaced client normally' }
    if (-not $oldProcess.WaitForExit(15000)) { throw 'Displaced client did not exit normally' }
    [ordered]@{name=$previousName; pid=$oldProcessId; normal_window_close=$true; utc=[DateTime]::UtcNow.ToString('o')} |
        ConvertTo-Json -Compress | Add-Content -LiteralPath 'build/certification/live/lifecycle.jsonl' -Encoding utf8
    Write-Output "Closed displaced client $previousName after $newName passed the hold"
    $previousName = $newName
}
