@echo off
setlocal EnableDelayedExpansion

call ..\..\argparser.bat %*
if errorlevel 1 exit /b 1

if exist publish (
    rmdir /s /q publish
)

if exist bin (
    rmdir /s /q bin
)

if exist obj (
    rmdir /s /q obj
)

if NOT [%OPT_A%]==[] (
    dotnet publish v2_aot.csproj -o publish --nologo
) else (
    dotnet publish v2.csproj -o publish --nologo
)

endlocal
