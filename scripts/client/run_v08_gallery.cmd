@echo off
setlocal
set "ROOT=%~dp0..\.."
set "EDITOR=C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor.exe"
set "PROJECT=%ROOT%\unreal\REAL33D\REAL33D.uproject"
set "CATALOG=%ROOT%\visual\qa\full_catalog_v08\experimental_catalog_runtime.json"
if not exist "%EDITOR%" ( echo MISSING: %EDITOR% & exit /b 2 )
if not exist "%PROJECT%" ( echo MISSING: %PROJECT% & exit /b 2 )
if not exist "%CATALOG%" ( echo MISSING: %CATALOG% & exit /b 2 )
echo Opening local V08 QA gallery. No catalog entry is approved by this mode.
"%EDITOR%" "%PROJECT%" -game -windowed -ResX=1440 -ResY=900 ^
  -real33d-gallery -real33d-experimental-catalog="%CATALOG%" -log -stdout -unattended=0
endlocal
