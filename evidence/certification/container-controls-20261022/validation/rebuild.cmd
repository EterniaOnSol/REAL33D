@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat"
call "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" REAL33DEditor Win64 Development -Project="C:\Users\dell\Desktop\fusion32\build\cert-container-independent-20261022\unreal\REAL33D\REAL33D.uproject" -WaitMutex -CompilerVersion=14.44.35207 -NoUBA -NoUBTMakefiles > "C:\Users\dell\Desktop\fusion32\build\cert-container-independent-20261022\build\certification\unreal-rebuild.txt" 2>&1
exit /b %ERRORLEVEL%
