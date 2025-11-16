@echo off
set BUILD_DIR=build-tests-mingw

if not exist %BUILD_DIR% mkdir %BUILD_DIR%
cmake -S . -B %BUILD_DIR% -G "MinGW Makefiles" -DDFH_BUILD_TESTS=ON
cmake --build %BUILD_DIR%
cd %BUILD_DIR%
ctest --output-on-failure
pause
