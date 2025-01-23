#!/bin/bash

usage() {
  echo ""
  echo "Invalid argument!"
  echo "Usage: $0 <implementation number>"
  echo ""
  echo "Implementations:"
  echo "1: C#"
  echo "2: C# AOT"
  exit 1
}

if [ "$1" = "1" ]; then
  cd csharp/version1
  bash build.sh
  bash run.sh
elif [ "$1" = "2" ]; then
  cd csharp/version1
  bash build.sh aot
  bash run.sh aot
else
  usage
fi
