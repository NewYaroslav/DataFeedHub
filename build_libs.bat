@echo off
mkdir build-libs
cd build-libs
cmake ../libs -G "MinGW Makefiles"
cmake --build . --config Release
pause