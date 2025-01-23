#!/bin/bash

rm -rf publish
rm -rf bin

if [ "$1" = "aot" ]; then
  dotnet publish v1_aot.csproj -o publish --nologo
else
  dotnet publish v1.csproj -o publish --nologo
fi
