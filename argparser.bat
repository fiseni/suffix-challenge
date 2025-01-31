@echo off
setlocal EnableDelayedExpansion

REM Author: Fati Iseni
REM Date: 2025-01-31
REM Script to parse both positional arguments and options combined.
REM Options can be placed anywhere while calling the script.
REM You can include the script in your batch file and call it with %*. Example:
REM call argparser.bat %*

set "arg_count=0"

:parse_args
if "%~1"=="" goto :done
if "%~1"=="-h" (
    set "opt_h=true"
    shift
    goto :parse_args
)
if "%~1"=="-s" (
    set "opt_s=true"
    shift
    goto :parse_args
)
if "%~1"=="-t" (
    set "opt_t=true"
    shift
    goto :parse_args
)
if "%~1"=="-a" (
    set "opt_a=true"
    shift
    goto :parse_args
)

REM Check if the positional argument starts with "-"
REM If it does, it means we have no such option.
echo %~1 | findstr "^-" >nul
if not errorlevel 1 (
    echo Invalid option: %1
    exit /b 1
)

REM We are shifting, so %~1 represents the current positional argument
set /a arg_count+=1
set "arg!arg_count!=%~1"
shift
goto :parse_args
:done

REM Build a command string to set variables after endlocal
set "EXPORT_VARS=set "ARG_COUNT=!arg_count!""
set "EXPORT_VARS=!EXPORT_VARS! & set "OPT_H=!opt_h!""
set "EXPORT_VARS=!EXPORT_VARS! & set "OPT_S=!opt_s!""
set "EXPORT_VARS=!EXPORT_VARS! & set "OPT_T=!opt_t!""
set "EXPORT_VARS=!EXPORT_VARS! & set "OPT_A=!opt_a!""

for /l %%i in (1,1,!arg_count!) do (
    set "EXPORT_VARS=!EXPORT_VARS! & set "ARG%%i=!arg%%i!""
)

endlocal & %EXPORT_VARS%
exit /b 0
