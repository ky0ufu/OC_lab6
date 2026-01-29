@echo off
setlocal

set BUILD_DIR=build-win
set CONFIG=Release
set MSYS2=C:\msys64\mingw64

set PATH=%MSYS2%\bin;%PATH%

cmake -S . -B %BUILD_DIR% -G "MinGW Makefiles" ^
  -DCMAKE_BUILD_TYPE=%CONFIG% ^
  -DCMAKE_PREFIX_PATH=%MSYS2%

cmake --build %BUILD_DIR%
