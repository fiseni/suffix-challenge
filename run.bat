@echo off
setlocal enabledelayedexpansion

set working_dir=%cd%
echo.

if "%~1"=="0" (
    call :all
    exit /b 0
) else if "%~1"=="1" (
    call :csharp_v1
    exit /b 0
) else if "%~1"=="2" (
    call :csharp_v1_aot
    exit /b 0
) else (
    call :usage
    exit /b 1
)

:csharp_v1
    cd  %working_dir%\csharp\v1
    call build.bat > nul
	hyperfine -i --output=pipe --runs 3 --warmup 2 --export-markdown %working_dir%\benchmarks\csharp_v1.md -n "C# v1" "run.bat"
    exit /b 0

:csharp_v1_aot
    cd  %working_dir%\csharp\v1
    call build.bat aot > nul
	hyperfine -i --output=pipe --runs 3 --warmup 2 --export-markdown %working_dir%\benchmarks\csharp_v1_aot.md -n "C# v1 AOT" "run.bat aot"
    exit /b 0

:all
    call :csharp_v1
    call :csharp_v1_aot
    exit /b 0

:usage
echo Invalid argument.
echo Usage: %~nx0 ^<implementation number^>
echo.
echo Implementations:
echo 0: All
echo 1: C# v1
echo 2: C# v1 AOT
echo.
exit /b 1

endlocal
