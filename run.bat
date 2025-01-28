@echo off
setlocal enabledelayedexpansion

set script_dir=%~dp0
set script_dir=%SCRIPT_DIR:~0,-1%
set "impl_list_file=%script_dir%\impl_list"

if not exist "%impl_list_file%" (
    echo Error: Implementation list file '%impl_list_file%' not found!
    exit /b 1
)

if "%~1"=="" goto :usage
if "%~1"=="0" goto :run_all
call :run_impl %1
exit /b 0

:usage
echo.
echo Invalid argument
echo Usage: %~nx0 ^<implementation number^>
echo.
echo Implementations:
for /f "usebackq tokens=1,2,* delims=:" %%A in ("%impl_list_file%") do (
    echo %%A: %%B
)
echo.
exit /b 1

:run_all
for /f "usebackq tokens=1,* delims=:" %%A in ("%impl_list_file%") do (
    if not "%%A"=="0" (
        call :run_impl %%A
    )
)
exit /b 0

:run_impl
set "found=false"
for /f "usebackq tokens=1,2,3,4,5,* delims=:" %%A in ("%impl_list_file%") do (
    if "%%A"=="%1" (
        if not exist "%script_dir%\benchmarks" (
            mkdir "%script_dir%\benchmarks"
        )
        echo.
        set "found=true"
        cd %script_dir%\%%C\%%D
        if errorlevel 1 exit /b 1
        echo Building "%%B" implementation...
        call build.bat %%E > nul
        echo Build completed.
        if exist results.txt (
            del results.txt
        )
        echo.
        if errorlevel 1 exit /b 1
        if "%%E"=="" (
            hyperfine -i --output=pipe --runs 5 --warmup 5 --export-markdown "%script_dir%\benchmarks\%%C_%%D.md" -n "%%B" "run.bat"
        ) else (
            hyperfine -i --output=pipe --runs 5 --warmup 5 --export-markdown "%script_dir%\benchmarks\%%C_%%D_%%E.md" -n "%%B" "run.bat %%E"
        )
        if errorlevel 1 exit /b 1
        call :compare_results "%script_dir%\data\expected.txt" "results.txt"
        echo -----------------------------------------------------------------------------------------------------------------
    )
)
if "!found!"=="false" goto :usage
exit /b 0

:compare_results
if not exist "%2" (
    echo File %2 not found!
    exit /b 0
)
fc "%1" "%2" > nul 2>&1
if errorlevel 1 (
    echo Test failed. The produced result file is different than the expected file.
) else (
    echo Test passed!
)
echo.
exit /b 0

endlocal
