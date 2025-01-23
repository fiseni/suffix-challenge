@echo off
setlocal enabledelayedexpansion

if "%~1"=="1" (
    cd csharp\version1
    call build.bat
    call run.bat
    exit /b 0
) else if "%~1"=="2" (
    cd csharp\version1
    call build.bat aot
    call run.bat aot
    exit /b 0
) else (
    call :usage
    exit /b 1
)

:usage
echo.
echo Invalid argument.
echo Usage: %~nx0 ^<implementation number^>
echo.
echo Implementations:
echo 1: C#
echo 2: C# AOT
echo.
exit /b 1

endlocal
