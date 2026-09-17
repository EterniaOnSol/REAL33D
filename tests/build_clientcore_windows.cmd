@echo off
rem Builds Protocol772Core natively on Windows with MSVC and runs its suites.
rem
rem Long recorded as UNVERIFIED: every previous run of these suites was GCC in
rem WSL. Unreal needs the Windows build, so this script is the check.
rem
rem OpenSSL comes from the engine's own ThirdParty tree, which is the same
rem libcrypto the Unreal module links, so this proves the exact combination the
rem game uses rather than an unrelated local OpenSSL.
rem
rem Run from a "x64 Native Tools Command Prompt", or call vcvars64.bat first.
rem
rem   tests\build_clientcore_windows.cmd [engine-root]
setlocal

set "ROOT=%~dp0.."
set "ENGINE=%~1"
if "%ENGINE%"=="" set "ENGINE=C:\Program Files\Epic Games\UE_5.8"

set "SSL=%ENGINE%\Engine\Source\ThirdParty\OpenSSL\1.1.1t"
set "SSLINC=%SSL%\include\Win64\VS2015"
set "SSLLIB=%SSL%\lib\Win64\VS2015\Release\libcrypto.lib"

if not exist "%SSLINC%\openssl\bn.h" (
  echo MISSING: %SSLINC%\openssl\bn.h
  echo Pass the engine root as the first argument if it is not at the default path.
  exit /b 2
)
if not exist "%SSLLIB%" ( echo MISSING: %SSLLIB% & exit /b 2 )

where cl.exe >nul 2>&1
if errorlevel 1 (
  echo MSVC not on PATH. Run this from a "x64 Native Tools Command Prompt",
  echo or call vcvars64.bat first.
  exit /b 3
)

set "OUT=%ROOT%\build\clientcore-windows"
set "LIBDIR=%OUT%\lib"
set "LIBDIR20=%OUT%\lib-cpp20"
if exist "%OUT%" rmdir /s /q "%OUT%"
mkdir "%LIBDIR%"
mkdir "%LIBDIR20%"

set "CORE=%ROOT%\clientcore"
set "SOURCES=crypto framed_connection framing gamelogin initial_world login map_scan movement movement_ledger object_types player_state tcp_transport worldstate worldview"
set "COMMON=/nologo /EHsc /W4 /WX /permissive- /MD /O2 /D_CRT_SECURE_NO_WARNINGS"
set "FLAGS=/std:c++17 %COMMON%"
set "INCS=/I"%CORE%\include" /I"%CORE%\tests" /I"%SSLINC%""
set "LIBS="%SSLLIB%" ws2_32.lib bcrypt.lib crypt32.lib advapi32.lib user32.lib"

rem Protocol772Core is written to C++17 and this is the build that proves it.
rem The suites below run against these objects.
echo === library sources, C++17 ===
cd /d "%LIBDIR%"
for %%F in (%SOURCES%) do (
  cl %FLAGS% /I"%CORE%\include" /I"%SSLINC%" /c "%CORE%\src\%%F.cpp" >nul || ( echo LIBRARY BUILD FAILED: %%F & exit /b 4 )
)
echo   ok

rem UE 5.8 will not build a module at C++17, so the archive Unreal links is the
rem same sources compiled again at C++20. Two compilations of one
rem implementation, never two implementations: the protocol still lives in
rem exactly one place. Compiling both ways also catches anything that changes
rem meaning between the two standards.
echo === library sources, C++20, for the Unreal link ===
cd /d "%LIBDIR20%"
for %%F in (%SOURCES%) do (
  cl /std:c++20 %COMMON% /I"%CORE%\include" /I"%SSLINC%" /c "%CORE%\src\%%F.cpp" >nul || ( echo C++20 LIBRARY BUILD FAILED: %%F & exit /b 4 )
)
lib /nologo /OUT:"%OUT%\protocol772core.lib" "%LIBDIR20%\*.obj" >nul
if errorlevel 1 ( echo LIBRARY ARCHIVE FAILED & exit /b 4 )
echo   ok, archived to %OUT%\protocol772core.lib

set "FAILED="
echo === suites ===
call :suite transport
call :suite crypto
call :suite login
call :suite gamelogin
call :suite initial_world
call :suite movement
call :suite player_state
call :suite worldview

if not "%FAILED%"=="" (
  echo.
  echo WINDOWS CLIENTCORE: FAIL --%FAILED%
  exit /b 5
)
echo.
echo WINDOWS CLIENTCORE: PASS
exit /b 0

:suite
set "NAME=%~1"
mkdir "%OUT%\%NAME%" >nul 2>&1
cd /d "%OUT%\%NAME%"
cl %FLAGS% %INCS% /Fe:%NAME%_tests.exe "%CORE%\tests\%NAME%_tests.cpp" "%LIBDIR%\*.obj" %LIBS% >build.log 2>&1
if errorlevel 1 (
  echo   %NAME%: BUILD FAILED, see %OUT%\%NAME%\build.log
  set "FAILED=%FAILED% %NAME%(build)"
  goto :eof
)
"%OUT%\%NAME%\%NAME%_tests.exe"
if errorlevel 1 set "FAILED=%FAILED% %NAME%(run)"
goto :eof
