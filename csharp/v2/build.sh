#!/bin/bash

source "../../argparser.sh"

rm -rf publish
rm -rf bin
rm -rf obj

if [ -n "$OPT_A" ]; then
  dotnet publish v2_aot.csproj -o publish --nologo
else
  dotnet publish v2.csproj -o publish --nologo
fi
