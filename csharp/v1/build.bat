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
    dotnet publish v1_aot.csproj -o publish --nologo
    move publish\v1_aot.exe publish\app.exe
) else (
    dotnet publish v1.csproj -o publish --nologo
    move publish\v1.exe publish\app.exe
)

endlocal
