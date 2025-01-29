@echo off

if "%~1"=="aot" (
    publish\v2_aot.exe ../../data/parts.txt ../../data/master-parts.txt results.txt
) else (
    publish\v2.exe ../../data/parts.txt ../../data/master-parts.txt results.txt
)
