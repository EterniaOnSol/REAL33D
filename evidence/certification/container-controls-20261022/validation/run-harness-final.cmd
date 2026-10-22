@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
if errorlevel 1 exit /b 1
set "PATH=C:\Program Files\Git\usr\bin;C:\Program Files\Git\mingw64\bin;%PATH%"
cd /d "C:\Users\dell\Desktop\fusion32\build\cert-container-independent-20261022"
"C:\Program Files\Git\bin\bash.exe" tests/regression_harness.sh --unreal --strict --logs build/certification/live --out build/certification/harness-final
exit /b %ERRORLEVEL%

