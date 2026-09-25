# Regression for Windows processes whose environment exposes duplicate key casing.
$ErrorActionPreference = 'Stop'
$launcher = Get-Content (Join-Path $PSScriptRoot '..\agent\real33d2d\run_agent.ps1') -Raw
if ($launcher -match 'Get-Item\s+"Env:\$name"') {
    throw 'Launcher must not query dynamic Env: paths (duplicate-key failure)'
}
if ($launcher -notmatch "GetEnvironmentVariable\(\`$name, 'Process'\)") {
    throw 'Launcher must use the Process environment API for dynamic settings'
}
$pathValue = [Environment]::GetEnvironmentVariable('PATH', 'Process')
if (-not $pathValue) { throw 'Process PATH lookup failed' }
Write-Output 'REAL33D_AGENT_LAUNCHER process-environment regression PASS'
