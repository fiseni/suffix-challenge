#!/bin/bash

rm -rf publish
rm -rf bin

if [ "$1" = "aot" ]; then
  dotnet publish version1-aot.csproj -o publish --nologo
else
  dotnet publish version1.csproj -o publish --nologo
fi
