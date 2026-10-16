@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2026\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b 1
set "PATH=C:\Program Files\Git\usr\bin;C:\Program Files\Git\bin;%PATH%"
"C:\Program Files\Git\bin\bash.exe" --login build/certification/run-harness.sh
exit /b %errorlevel%
