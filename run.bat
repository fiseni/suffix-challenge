@echo off
setlocal enabledelayedexpansion

set working_dir=%cd%

echo.

if "%~1"=="0" (
    call :all
    exit /b 0
) else if "%~1"=="1" (
    call :csharp_version1
    exit /b 0
) else if "%~1"=="2" (
    call :csharp_version1_aot
    exit /b 0
) else (
    call :usage
    exit /b 1
)

:csharp_version1
    cd  %working_dir%
    cd csharp\version1
    call build.bat > nul
	hyperfine -i --output=pipe --runs 3 --warmup 2 -n "C#" "run.bat"
    exit /b 0

:csharp_version1_aot
    cd  %working_dir%
    cd csharp\version1
    call build.bat aot > nul
	hyperfine -i --output=pipe --runs 3 --warmup 2 -n "C# AOT" "run.bat aot"
    exit /b 0

:all
    cd  %working_dir%
    call :csharp_version1
    call :csharp_version1_aot
    exit /b 0

:usage
echo.
echo Invalid argument.
echo Usage: %~nx0 ^<implementation number^>
echo.
echo Implementations:
echo 0: All
echo 1: C#
echo 2: C# AOT
echo.
exit /b 1

endlocal
