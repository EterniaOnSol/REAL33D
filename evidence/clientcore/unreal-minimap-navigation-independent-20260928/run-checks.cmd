@echo off
setlocal
set "ROOT=C:\Users\dell\Desktop\fusion32\build\unreal-minimap-certification-20260928"
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
call "%ROOT%\tests\build_clientcore_windows.cmd" >"%ROOT%\build\certification-evidence\clientcore-build-tests.txt" 2>&1
if errorlevel 1 exit /b 1
cd /d "%ROOT%\build\certification-evidence"
cl /nologo /EHsc /W4 /WX /permissive- /std:c++17 /Fe:minimap_view_tests.exe "%ROOT%\tests\real33d_minimap_view_tests.cpp" >view-build.txt 2>&1
if errorlevel 1 exit /b 2
minimap_view_tests.exe >view-tests.txt 2>&1
if errorlevel 1 exit /b 3
call "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" REAL33DEditor Win64 Development "-Project=%ROOT%\unreal\REAL33D\REAL33D.uproject" -WaitMutex >unreal-build.txt 2>&1
exit /b %errorlevel%
