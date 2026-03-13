@echo off
if not exist build mkdir build
cd build
if not exist "reconfigure.bat" (
    echo cd ..> reconfigure.bat
    echo configure.bat>> reconfigure.bat
)
cmake -G "Visual Studio 18 2026" -A x64 ..