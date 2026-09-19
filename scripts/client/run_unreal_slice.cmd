@echo off
rem Runs the REAL33D Unreal client against the sanitized Fusion32 runtime.
rem
rem The runtime lives inside WSL because that is where the server is prepared
rem and started. Unreal runs on Windows and reaches it two ways:
rem   - the secrets and objects.srv are read through the \\wsl.localhost share
rem   - the sockets go to 127.0.0.1, which WSL2 forwards into the distribution
rem
rem The client runs uncooked, through the editor executable in -game mode, so
rem no packaging step stands between a source change and a live run.
rem
rem   scripts\client\run_unreal_slice.cmd [account-letter] [runtime-path]
rem
rem account-letter defaults to B, because Player A is the original Tibia.exe.
setlocal

set "ROOT=%~dp0..\.."
set "ACCOUNT=%~1"
if "%ACCOUNT%"=="" set "ACCOUNT=B"

set "RUNTIME=%~2"
if "%RUNTIME%"=="" set "RUNTIME=\\wsl.localhost\Ubuntu-26.04\var\lib\fusion32-server-baseline-772-0"

set "ENGINE=C:\Program Files\Epic Games\UE_5.8"
set "EDITOR=%ENGINE%\Engine\Binaries\Win64\UnrealEditor.exe"
set "PROJECT=%ROOT%\unreal\REAL33D\REAL33D.uproject"
set "EVIDENCE=%ROOT%\evidence\clientcore\unreal-slice"

if not exist "%EDITOR%" ( echo MISSING: %EDITOR% & exit /b 2 )
if not exist "%PROJECT%" ( echo MISSING: %PROJECT% & exit /b 2 )
if not exist "%RUNTIME%\secrets\credentials.env" (
  echo MISSING: %RUNTIME%\secrets\credentials.env
  echo Prepare and start the server first:
  echo   wsl -e bash scripts/server/prepare_wsl.sh /mnt/c/Users/dell/Desktop/fusion32
  echo   wsl -e bash scripts/server/start_wsl.sh
  exit /b 2
)

if not exist "%EVIDENCE%" mkdir "%EVIDENCE%"

echo Account %ACCOUNT%, runtime %RUNTIME%
echo Walk with WASD or the arrow keys. F9 writes a labelled evidence snapshot.
echo.

"%EDITOR%" "%PROJECT%" -game -windowed -ResX=1280 -ResY=720 ^
  -real33d-runtime="%RUNTIME%" ^
  -real33d-account=%ACCOUNT% ^
  -real33d-host=127.0.0.1 ^
  -real33d-loginport=7171 ^
  -real33d-evidence="%EVIDENCE%" ^
  -log -stdout -unattended=0

endlocal
