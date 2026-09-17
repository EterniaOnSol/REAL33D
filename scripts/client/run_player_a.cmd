@echo off
rem Brings up Player A on the ORIGINAL Tibia 7.72 client.
rem
rem This is the half of the acceptance test a script cannot finish: the client
rem is a GUI from 2006 and someone has to type the account number and password
rem into it. Everything up to that point is done here.
rem
rem Order matters. The IP Changer patches a client that is already running: it
rem looks for a window of class TibiaClient, checks the version marker and
rem writes the endpoints and the RSA modulus into that live process. So the
rem client starts first and is patched second.
rem
rem The password is never printed here and never written anywhere. The
rem credentials file is opened in Notepad so it can be read and copied
rem directly, and it stays inside the sanitized runtime.
rem
rem   scripts\client\run_player_a.cmd [runtime-path]
setlocal

set "ROOT=%~dp0..\.."
set "RUNTIME=%~1"
if "%RUNTIME%"=="" set "RUNTIME=\\wsl.localhost\Ubuntu-26.04\tmp\fusion32-server-baseline-772-0"

set "APP=%ROOT%\build\classic-client-772\app"
set "IPC=%ROOT%\build\ipchanger"

if not exist "%APP%\Tibia.exe" ( echo MISSING: %APP%\Tibia.exe & exit /b 2 )
if not exist "%IPC%\ipchanger.exe" (
  echo MISSING: %IPC%\ipchanger.exe
  echo Build it from an x86 Native Tools Command Prompt:
  echo   tests\build_ipchanger_windows.cmd
  exit /b 2
)
if not exist "%IPC%\servers.txt" (
  echo MISSING: %IPC%\servers.txt
  echo A runtime reset changes the RSA modulus, so regenerate it:
  echo   wsl -e bash scripts/client/prepare_fusion32_ipchanger_wsl.sh /mnt/c/Users/dell/Desktop/fusion32
  exit /b 2
)

echo Starting the original Tibia 7.72 client...
start "" /D "%APP%" "%APP%\Tibia.exe"

rem Give the client time to create its window before the patch looks for it.
ping -n 6 127.0.0.1 >nul

rem Run from the IP Changer's own directory: it reads servers.txt relative to
rem the working directory. The executable is still named by full path, because
rem the current directory is not on the search path on a hardened system.
echo Patching it to reach Fusion32 at 127.0.0.1:7171...
cd /d "%IPC%"
"%IPC%\ipchanger.exe" fusion32
if errorlevel 1 (
  echo.
  echo The patch did not apply. Make sure the Tibia window is open, then run:
  echo   cd /d "%IPC%" ^&^& "%IPC%\ipchanger.exe" fusion32
)

echo.
echo Opening the credentials file. Log in as ACCOUNT_A_ID with ACCOUNT_A_PASSWORD
echo and pick the character "Test Player A".
start "" notepad.exe "%RUNTIME%\secrets\credentials.env"

endlocal
