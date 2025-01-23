@echo off

if "%~1"=="aot" (
    publish\version1-aot.exe ../../data/parts.txt ../../data/master-parts.txt results.txt
) else (
    publish\version1.exe ../../data/parts.txt ../../data/master-parts.txt results.txt
)
