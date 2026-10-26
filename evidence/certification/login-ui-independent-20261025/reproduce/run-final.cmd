@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
cd /d C:\Users\dell\Desktop\fusion32\build\cert-login-ui-independent-20261025
"C:\Program Files\Git\bin\bash.exe" tests/regression_harness.sh --unreal --strict --objects-srv /c/Users/dell/Desktop/fusion32/build/cert-login-ui-independent-20261025/build/runtime-metadata/objects.srv --out build/certification/final --logs build/certification/live
exit /b %ERRORLEVEL%

