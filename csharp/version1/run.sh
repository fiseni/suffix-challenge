#!/bin/bash

if [ "$1" = "aot" ]; then
  ./publish/version1-aot.exe ../../data/parts.txt ../../data/master-parts.txt results.txt
else
  ./publish/version1.exe ../../data/parts.txt ../../data/master-parts.txt results.txt
fi
