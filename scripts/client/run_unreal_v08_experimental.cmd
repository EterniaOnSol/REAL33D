@echo off
rem Runs the real Fusion32 world with the local V08 QA mapping explicitly on.
setlocal
set "ROOT=%~dp0..\.."
set "ACCOUNT=%~1"
if "%ACCOUNT%"=="" set "ACCOUNT=B"
set "RUNTIME=%~2"
if "%RUNTIME%"=="" set "RUNTIME=\\wsl.localhost\Ubuntu-26.04\var\lib\fusion32-server-baseline-772-0"
set "EDITOR=C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
set "PROJECT=%ROOT%\unreal\REAL33D\REAL33D.uproject"
set "CATALOG=%ROOT%\visual\qa\full_catalog_v08\experimental_catalog_runtime.json"
set "EVIDENCE=%ROOT%\evidence\clientcore\unreal-slice\v08-experimental"
if not exist "%EDITOR%" ( echo MISSING: %EDITOR% & exit /b 2 )
if not exist "%PROJECT%" ( echo MISSING: %PROJECT% & exit /b 2 )
if not exist "%CATALOG%" ( echo MISSING: %CATALOG% & exit /b 2 )
if not exist "%RUNTIME%\secrets\credentials.env" ( echo MISSING: sanitized runtime & exit /b 2 )
if not exist "%EVIDENCE%" mkdir "%EVIDENCE%"
echo TEST catalog enabled. TEST_IMPORTED is not APPROVED, READY, or production INTEGRATED.
"%EDITOR%" "%PROJECT%" -game -windowed -ResX=1280 -ResY=720 -real33d-experimental-catalog="%CATALOG%" -real33d-runtime="%RUNTIME%" -real33d-account=%ACCOUNT% -real33d-host=127.0.0.1 -real33d-loginport=7171 -real33d-evidence="%EVIDENCE%" -log -stdout -unattended=0
endlocal
