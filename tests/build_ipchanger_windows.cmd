@echo off
setlocal

set "REPO=%~dp0.."
set "SOURCE=%REPO%\reference\ipchanger"
set "OUTPUT=%REPO%\build\ipchanger"

if not exist "%OUTPUT%" mkdir "%OUTPUT%"
pushd "%OUTPUT%"

cl /nologo /W3 /WX /Zi /D_CRT_SECURE_NO_WARNINGS=1 /Fe:ipchanger.exe /Fo:ipchanger.obj /Fd:ipchanger.pdb "%SOURCE%\ipchanger.cc" /link /subsystem:console /incremental:no /opt:ref /dynamicbase user32.lib
if errorlevel 1 exit /b 1

cl /nologo /W3 /WX /Zi /D_CRT_SECURE_NO_WARNINGS=1 /Fe:memscan.exe /Fo:memscan.obj /Fd:memscan.pdb "%SOURCE%\memscan.cc" /link /subsystem:console /incremental:no /opt:ref /dynamicbase user32.lib
if errorlevel 1 exit /b 1

popd
endlocal
