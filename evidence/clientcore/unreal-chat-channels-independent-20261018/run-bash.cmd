@echo off
call "C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools\VC\Auxiliary\Build\vcvars64.bat" >nul
if errorlevel 1 exit /b %ERRORLEVEL%
set "PATH=C:\Program Files\Git\bin;C:\Program Files\Git\usr\bin;%PATH%"
"C:\Program Files\Git\bin\bash.exe" --login %1
exit /b %ERRORLEVEL%
