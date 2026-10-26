@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
cd /d C:\Users\dell\Desktop\fusion32\build\cert-login-ui-recert-20261026
"C:\Program Files\Git\bin\bash.exe" tests/regression_harness.sh --unreal --strict --runtime //wsl.localhost/Ubuntu-26.04/var/lib/fusion32-server-baseline-772-0 --objects-srv build/runtime-metadata/objects.srv --out build/recert/final-harness --logs build/recert/live
exit /b %ERRORLEVEL%

