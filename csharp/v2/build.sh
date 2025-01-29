#!/bin/bash

rm -rf publish
rm -rf bin
rm -rf obj

if [ "$1" = "aot" ]; then
  dotnet publish v2_aot.csproj -o publish --nologo
else
  dotnet publish v2.csproj -o publish --nologo
fi
