@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
if errorlevel 1 exit /b %errorlevel%
set "PATH=C:\Program Files\Git\usr\bin;C:\Program Files\Git\bin;C:\Users\dell\AppData\Local\Programs\Python\Python311;%PATH%"
"C:\Program Files\Git\bin\bash.exe" tests/regression_harness.sh --out build/certification/harness --unreal
exit /b %errorlevel%
