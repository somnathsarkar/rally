@echo off
cls
mkdir build
mkdir build\debug
mkdir build\release
pushd build\debug
cmake -DCMAKE_BUILD_TYPE=Debug ../..
cmake --build . || exit /b 1
pushd tests\Debug
.\rallytest.exe || exit /b 1
popd
popd

REM Release build
::pushd build\release
::cmake -DCMAKE_BUILD_TYPE=Release ../..
::cmake --build . || exit /b 1
::popd