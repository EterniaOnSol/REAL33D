@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
if errorlevel 1 exit /b 1
set "PATH=C:\Users\dell\AppData\Local\Programs\Python\Python311;C:\Windows\System32;C:\Program Files\Git\usr\bin;C:\Program Files\Git\mingw64\bin;%PATH%"
set "PYTHON=C:/Users/dell/AppData/Local/Programs/Python/Python311/python.exe"
cd /d "C:\Users\dell\Desktop\fusion32\build\cert-player-death-20261023"
"C:\Program Files\Git\bin\bash.exe" tests/regression_harness.sh --unreal --strict --logs build/certification/live --objects-srv /c/Users/dell/Desktop/fusion32/build/cert-player-death-20261023/build/certification/objects.srv --out build/certification/final-harness
exit /b %ERRORLEVEL%



