@echo off

if exist publish (
    rmdir /s /q publish
)

if exist bin (
    rmdir /s /q bin
)

if "%~1"=="aot" (
    dotnet publish v1_aot.csproj -o publish --nologo
) else (
    dotnet publish v1.csproj -o publish --nologo
)
