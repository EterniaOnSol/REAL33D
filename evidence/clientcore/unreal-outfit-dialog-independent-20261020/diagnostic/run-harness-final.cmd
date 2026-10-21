@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
if errorlevel 1 exit /b 1
"C:\Program Files\Git\bin\bash.exe" tests/regression_harness.sh --out build/certification/harness-final --logs build/certification/live-20261020T221312
exit /b %ERRORLEVEL%




