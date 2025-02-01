#!/bin/bash

source "../../argparser.sh"

rm -rf publish
rm -rf bin
rm -rf obj

if [ -n "$OPT_A" ]; then
  dotnet publish v1_aot.csproj -o publish --nologo
  mv publish/v1_aot publish/app
else
  dotnet publish v1.csproj -o publish --nologo
  mv publish/v1 publish/app
fi
