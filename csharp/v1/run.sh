#!/bin/bash

if [ "$1" = "aot" ]; then
  ./publish/v1_aot.exe ../../data/parts.txt ../../data/master-parts.txt results.txt
else
  ./publish/v1.exe ../../data/parts.txt ../../data/master-parts.txt results.txt
fi
