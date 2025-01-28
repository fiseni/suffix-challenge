@echo off

if exist publish (
    rmdir /s /q publish
)

if exist bin (
    rmdir /s /q bin
)

if "%~1"=="aot" (
    dotnet publish v2_aot.csproj -o publish --nologo
) else (
    dotnet publish v2.csproj -o publish --nologo
)
