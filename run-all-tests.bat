@echo off
setlocal enabledelayedexpansion

rem Переход в корень репозитория (папка, где лежит этот батник)
cd /d "%~dp0"

echo [MSVC] Build Debug...
cmake --build build-msvc --config Debug
if errorlevel 1 goto :fail

echo.
echo [MSVC] Run ctest...
cd build-msvc
ctest -C Debug --output-on-failure
if errorlevel 1 goto :fail

cd /d "%~dp0"

echo.
echo [MinGW] Run build-tests-mingw.bat...
call build-tests-mingw.bat
if errorlevel 1 goto :fail

echo.
echo All tests (MSVC + MinGW) PASSED.
exit /b 0

:fail
echo.
echo Tests FAILED.
exit /b 1