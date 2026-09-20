param(
    [string]$Source = 'C:\Users\dell\3DTIBIA_leo',
    [ValidateRange(1, 1000)][int]$BatchSize = 250,
    [switch]$RetryFailed
)

$ErrorActionPreference = 'Stop'
$worker = Join-Path $PSScriptRoot 'import_v08_catalog_unreal.ps1'
$total = 4913
for ($start = 0; $start -lt $total; $start += $BatchSize) {
    $limit = [Math]::Min($BatchSize, $total - $start)
    Write-Host "REAL33D V08 batch start=$start limit=$limit"
    & $worker -Source $Source -Start $start -Limit $limit -RetryFailed:$RetryFailed
    if ($LASTEXITCODE -ne 0) { throw "V08 batch start=$start failed with exit code $LASTEXITCODE" }
}
Write-Host 'REAL33D V08 full catalog pass complete.'
