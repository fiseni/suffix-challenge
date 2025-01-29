#!/bin/bash

if [ "$1" = "aot" ]; then
  ./publish/v2_aot ../../data/parts.txt ../../data/master-parts.txt results.txt
else
  ./publish/v2 ../../data/parts.txt ../../data/master-parts.txt results.txt
fi
