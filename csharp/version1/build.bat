@echo off

if exist publish (
    rmdir /s /q publish
)

if exist bin (
    rmdir /s /q bin
)

if "%~1"=="aot" (
    dotnet publish version1-aot.csproj -o publish --nologo
) else (
    dotnet publish version1.csproj -o publish --nologo
)
