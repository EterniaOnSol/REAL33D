# Launch the opt-in REAL33D2D agent against a prepared local Fusion32 QA runtime.
#
# No deployment path, character or credential is hardcoded here. Everything
# local comes from agent.local.env (gitignored; see agent.local.env.example) or
# from the environment. The account and password are read from the QA runtime's
# own generated secrets file into this process only: they are never echoed,
# never passed as command arguments, and never written to a file by this script.
param(
    [ValidateSet('bridge', 'legacy')][string]$Mode = 'bridge',
    [ValidateSet('mock', 'ollama')][string]$Brain = 'mock',
    [string]$Character,
    [string]$Trace,
    [switch]$Memory,
    [string]$MemoryDir
)

$ErrorActionPreference = 'Stop'

$localEnv = Join-Path $PSScriptRoot 'agent.local.env'
if (Test-Path $localEnv) {
    foreach ($line in Get-Content $localEnv) {
        if ($line -match '^\s*#' -or $line -notmatch '=') { continue }
        $pair = $line -split '=', 2
        $name = $pair[0].Trim()
        $value = $pair[1].Trim()
        if ($name -and $value -and -not [Environment]::GetEnvironmentVariable($name, 'Process')) {
            [Environment]::SetEnvironmentVariable($name, $value, 'Process')
        }
    }
}

function Require-Setting([string]$name) {
    $value = [Environment]::GetEnvironmentVariable($name, 'Process')
    if (-not $value) {
        throw "$name is not set. Copy agent.local.env.example to agent.local.env and fill it in."
    }
    return $value
}

$clientRoot = Require-Setting 'R33D_CLIENT_ROOT'
$distro = Require-Setting 'R33D_WSL_DISTRO'
$credentialPath = Require-Setting 'R33D_QA_CREDENTIALS'
if (-not $Character) { $Character = Require-Setting 'R33D_QA_CHARACTER' }
if ($Character -notmatch '^[A-Za-z0-9_]+$') { throw 'Character must be alphanumeric' }

$exe = Join-Path $clientRoot 'Release\otclient.exe'
if (-not (Test-Path $exe)) { throw "Client binary not found: $exe" }

if ($Mode -eq 'bridge' -and $Brain -ne 'mock') {
    throw 'Bridge certification is mock-only. Use -Mode legacy to exercise another brain.'
}

$lines = & wsl.exe -d $distro -- cat $credentialPath
if ($LASTEXITCODE -ne 0) { throw 'Could not read local QA credentials' }
$data = @{}
foreach ($line in $lines) {
    $pair = $line -split '=', 2
    if ($pair.Length -eq 2) { $data[$pair[0]] = $pair[1] }
}
$account = $data["ACCOUNT_${Character}_ID"]
$password = $data["ACCOUNT_${Character}_PASSWORD"]
$data = $null
$lines = $null
if (-not $account -or -not $password) { throw 'QA account fields missing' }

try {
    $env:R33D_ACC = $account
    $env:R33D_PW = $password
    $env:R33D_AGENT_AUTOLOGIN = '1'
    if ($Mode -eq 'bridge') {
        $env:R33D_AGENT_MODE = '1'
        $env:R33D_AGENT_BRAIN = 'mock'
        if (-not $Trace) { $Trace = $env:R33D_AGENT_TRACE }
        if (-not $Trace) { $Trace = Join-Path $clientRoot 'real33d_agent_trace.jsonl' }
        $env:R33D_AGENT_TRACE = $Trace
        if ($Memory) {
            # Persistent memory is opt-in on top of bridge mode. Without the
            # switch no memory file is read or written at all.
            if (-not $MemoryDir) { $MemoryDir = $env:R33D_AGENT_MEMORY_DIR }
            if (-not $MemoryDir) { $MemoryDir = Join-Path $clientRoot 'agent_memory' }
            if (-not (Test-Path $MemoryDir)) {
                New-Item -ItemType Directory -Path $MemoryDir -Force | Out-Null
            }
            $env:R33D_AGENT_MEMORY = '1'
            $env:R33D_AGENT_MEMORY_DIR = $MemoryDir
        }
    } else {
        $env:R33D_AGENT = '1'
        $env:R33D_AGENT_BRAIN = $Brain
    }
    Remove-Item Env:R33D_ACCEPTANCE,Env:R33D_MOVEONLY,Env:R33D_TAPTEST,Env:R33D_MANUALTAP -ErrorAction SilentlyContinue
    $process = Start-Process -FilePath $exe -WorkingDirectory $clientRoot -PassThru
    Write-Output "REAL33D2D agent started: mode=$Mode brain=$Brain pid=$($process.Id)"
    if ($Mode -eq 'bridge') {
        Write-Output "trace=$Trace"
        if ($Memory) { Write-Output "memory=$MemoryDir" }
    }
}
finally {
    $account = $null
    $password = $null
    Remove-Item Env:R33D_ACC,Env:R33D_PW,Env:R33D_AGENT,Env:R33D_AGENT_MODE,`
        Env:R33D_AGENT_AUTOLOGIN,Env:R33D_AGENT_BRAIN,Env:R33D_AGENT_MEMORY `
        -ErrorAction SilentlyContinue
}
