@echo off
call "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" REAL33DEditor Win64 Development -Project="C:\Users\dell\Desktop\fusion32\build\cert-outfit-dialog-20261020\unreal\REAL33D\REAL33D.uproject" -WaitMutex -CompilerVersion=14.44.35207 -NoUBA -MaxParallelActions=1
exit /b %ERRORLEVEL%
