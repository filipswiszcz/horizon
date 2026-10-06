@echo off

set "mode=%1"

if "%mode%"=="dev" goto dev
if "%mode%"=="release" goto release

echo BUILD OPTIONS: dev, release
goto eof

:dev
set "flags=/Zi /O2 /MDd /W4"

:release
set "flags=/O2 /MD /W4"

:compile
if not exist bin mkdir bin
if exist build rmdir /s /q build
mkdir build

cl.exe /nologo /std:c++17 %flags% ^
    /I./lib ^
    src\*.cpp ^
    /Fobuild\ ^
    /Fdbuild\ ^
    /Febuild\device.exe ^
    /link

if %errorlevel% neq 0 (
    echo BUILD ERROR
    exit /b %errorlevel%
)

copy /y build\device.exe bin\ >nul

echo BUILD COMPLETE

:eof