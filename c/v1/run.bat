@echo off
setlocal EnableDelayedExpansion

call ..\..\argparser.bat %*
if errorlevel 1 exit /b 1

set parts_file="../../data/parts.txt"
set master_parts_file="../../data/master-parts.txt"
set results_file="results.txt"

if %ARG_COUNT% GEQ 3 (
  set parts_file=%ARG1%
  set master_parts_file=%ARG2%
  set results_file=%ARG3%
)

publish\v1.exe %parts_file% %master_parts_file% %results_file%

endlocal
