@echo off
setlocal enabledelayedexpansion

REM Author: Fati Iseni
REM Date: 2025-01-31

REM The full path of this script. Removes the trailing backslash.
set script_dir=%~dp0
set script_dir=%script_dir:~0,-1%

REM Parse positional arguments and options.
call argparser.bat %*
if errorlevel 1 exit /b 1

REM Check if the implementation list file exists.
set impl_list_file=%script_dir%\impl_list
if not exist %impl_list_file% (
    echo Error: Implementation list file '%impl_list_file%' not found!
    exit /b 1
)

REM If -t option is set, use test data files.
set results_file_name=results.txt
set parts_file=%script_dir%\data\parts.txt
set master_parts_file=%script_dir%\data\master-parts.txt
set expected_file=%script_dir%\data\expected.txt

if NOT [%OPT_T%]==[] (
    set parts_file=%script_dir%\data\test-parts.txt
    set master_parts_file=%script_dir%\data\test-master-parts.txt
    set expected_file=%script_dir%\data\test-expected.txt
)

REM If -s or -t options are set, do not run benchmarks.
if NOT [%OPT_S%]==[] set is_simple_run=true
if NOT [%OPT_T%]==[] set is_simple_run=true

REM If -b is set, run without building.
REM Usually used in combination with -s for repetitive runs.
if NOT [%OPT_B%]==[] set nobuild=true

REM Main execution.
if NOT [%OPT_H%]==[] goto :show_help
if "%ARG1%"=="" goto :show_help
if "%ARG1%"=="0" goto :run_all
call :run_impl %ARG1%
exit /b 0

REM ####################################################################
:show_help
    echo.
    echo Usage: %~nx0 ^<implementation number^>
    echo.
    echo Implementations:
    for /f "usebackq tokens=1,2,* delims=:" %%A in (%impl_list_file%) do (
        set impl_num=%%A
        if NOT "!impl_num:~0,1!"=="#" echo %%A: %%B
    )
    echo.
exit /b 1

REM ####################################################################
:run_all
    for /f "usebackq tokens=1,* delims=:" %%A in (%impl_list_file%) do (
        set impl_num=%%A
        set call_impl=true
        if "!impl_num!"=="0" set call_impl=false;
        if "!impl_num:~0,1!"=="#" set call_impl=false;
        if "!call_impl!"=="true" (
            call :run_impl !impl_num!
        )
    )
exit /b 0

REM ####################################################################
:run_impl
    set found=false
    for /f "usebackq tokens=1,2,3,4,5,* delims=:" %%A in (%impl_list_file%) do (
        if "%%A"=="%1" (

            REM We will switch to impl dir just in case if someone assumes that in impl scripts.
            echo.
            set found=true
            set impl_dir=%script_dir%\%%C\%%D
            cd !impl_dir!
            if errorlevel 1 exit /b 1

            REM We will export hyperfine results as md files in this directory.
            if not exist %script_dir%\benchmarks (
                mkdir %script_dir%\benchmarks
            )

            REM Always use absolute paths, just in case.
            if NOT "%nobuild%"=="true" (
                echo Building "%%B" implementation...
                call !impl_dir!\build.bat %%E > nul
                if errorlevel 1 exit /b 1
                echo Build completed.
                echo.
            )

            REM The output results file is always in the impl directory.
            set results_file=!impl_dir!\%results_file_name%
            if exist !results_file! (
                del !results_file!
            )
            if errorlevel 1 exit /b 1

            REM If -s or -t options are set, we will just execute the script.
            if "%is_simple_run%"=="true" (
                !impl_dir!\run.bat %parts_file% %master_parts_file% !results_file! %%E
            ) else (
                hyperfine -i --output=pipe --runs 10 --warmup 3 --export-markdown "%script_dir%\benchmarks\%%A.md" -n "%%B" "!impl_dir!\run.bat %parts_file% %master_parts_file% !results_file! %%E"
            )
            if errorlevel 1 exit /b 1

            REM Check for the correctness of results.
            call :compare_results %expected_file% !results_file!
            echo -----------------------------------------------------------------------------------------------------------------
        )
    )
    if "!found!"=="false" goto :show_help
exit /b 0

REM ####################################################################
:compare_results
    if not exist %2 (
        echo File %2 not found!
        exit /b 0
    )
    fc %1 %2 > nul 2>&1
    if errorlevel 1 (
        echo [91mTest failed[0m. The produced result file is different than the expected file.
    ) else (
        echo [92mTest passed![0m
    )
    echo.
exit /b 0

endlocal
REM ####################################################################
