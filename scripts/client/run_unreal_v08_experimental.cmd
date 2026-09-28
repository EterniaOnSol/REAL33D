@echo off
rem Runs the real Fusion32 world with the local V08 QA mapping explicitly on.
rem The third argument may point at a WideWorld cache from another worktree.
rem The fourth/fifth may point at that worktree's V08 catalog and previews.
setlocal
set "ROOT=%~dp0..\.."
set "ACCOUNT=%~1"
if "%ACCOUNT%"=="" set "ACCOUNT=B"
set "RUNTIME=%~2"
if "%RUNTIME%"=="" set "RUNTIME=\\wsl.localhost\Ubuntu-26.04\var\lib\fusion32-server-baseline-772-0"
set "EDITOR=C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
set "PROJECT=%ROOT%\unreal\REAL33D\REAL33D.uproject"
set "CATALOG=%ROOT%\visual\qa\full_catalog_v08\experimental_catalog_runtime.json"
if not "%~4"=="" set "CATALOG=%~4"
set "CACHE=%ROOT%\unreal\REAL33D\Saved\WideWorldCache"
if not "%~3"=="" set "CACHE=%~3"
set "PREVIEWS=%ROOT%\visual\reference_pack\previews\item"
if not "%~5"=="" set "PREVIEWS=%~5"
set "RADIUS=64"
if not "%~6"=="" set "RADIUS=%~6"
set "EVIDENCE=%ROOT%\evidence\clientcore\unreal-slice\v08-experimental"
if not exist "%EDITOR%" ( echo MISSING: %EDITOR% & exit /b 2 )
if not exist "%PROJECT%" ( echo MISSING: %PROJECT% & exit /b 2 )
if not exist "%CATALOG%" ( echo MISSING: %CATALOG% & exit /b 2 )
if not exist "%CACHE%\*.wws" (
  echo MISSING: WideWorld sector cache in %CACHE%
  echo Generate it with scripts\client\build_wide_world_cache.py before launching.
  exit /b 2
)
if not exist "%ROOT%\unreal\REAL33D\Content\Experimental\V08" (
  echo MISSING: local imported V08 meshes in the project Content directory.
  exit /b 2
)
if not exist "%RUNTIME%\secrets\credentials.env" ( echo MISSING: sanitized runtime & exit /b 2 )
if not exist "%EVIDENCE%" mkdir "%EVIDENCE%"
echo TEST catalog and WideWorld radius %RADIUS% enabled. TEST_IMPORTED is not APPROVED, READY, or production INTEGRATED.
"%EDITOR%" "%PROJECT%" -game -windowed -ResX=1280 -ResY=720 -real33d-experimental-catalog="%CATALOG%" -real33d-wide-world-cache="%CACHE%" -real33d-wide-world-previews="%PREVIEWS%" -real33d-visual-radius=%RADIUS% -real33d-runtime="%RUNTIME%" -real33d-account=%ACCOUNT% -real33d-host=127.0.0.1 -real33d-loginport=7171 -real33d-evidence="%EVIDENCE%" -log -stdout -unattended=0
endlocal
