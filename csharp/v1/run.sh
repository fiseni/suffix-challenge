#!/bin/bash

if [ "$1" = "aot" ]; then
  ./publish/v1_aot ../../data/parts.txt ../../data/master-parts.txt results.txt
else
  ./publish/v1 ../../data/parts.txt ../../data/master-parts.txt results.txt
fi
