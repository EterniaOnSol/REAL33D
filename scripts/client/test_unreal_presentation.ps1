param([string]$EvidenceDirectory = '')
$ErrorActionPreference = 'Stop'
$polishRoot = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..'))
$polishEvidence = if ($EvidenceDirectory) { [IO.Path]::GetFullPath($EvidenceDirectory) } else {
 Join-Path $polishRoot ('build\unreal-world-presentation-polish-001\checks-' + (Get-Date -Format 'yyyyMMddTHHmmss'))
}
New-Item -ItemType Directory -Path $polishEvidence -Force | Out-Null
$polishArgs = @(
 ('"' + (Join-Path $polishRoot 'unreal\REAL33D\REAL33D.uproject') + '"'),
 '-unattended', '-nop4', '-nullrhi', '-nosplash',
 '-ExecCmds="Automation RunTests REAL33D.Presentation+REAL33D.WideWorld"',
 '-TestExit="Automation Test Queue Empty"',
 ('-ReportExportPath="' + (Join-Path $polishEvidence 'automation-report') + '"'),
 ('-abslog="' + (Join-Path $polishEvidence 'automation-tests.log') + '"')
)
$polishProcess = Start-Process -FilePath 'C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' -ArgumentList $polishArgs -WindowStyle Hidden -PassThru -Wait
exit $polishProcess.ExitCode
