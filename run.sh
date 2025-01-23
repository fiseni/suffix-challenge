#!/bin/bash

working_dir="$(pwd)"

echo ""

usage() {
  echo ""
  echo "Invalid argument!"
  echo "Usage: $0 <implementation number>"
  echo ""
  echo "Implementations:"
  echo "0: All"
  echo "1: C#"
  echo "2: C# AOT"
  exit 1
}

csharp_version1() {
  cd "$working_dir"
  cd csharp/version1
  bash build.sh >/dev/null
  hyperfine -i --output=pipe --runs 3 --warmup 2 -n "C#" "bash run.sh"
}

csharp_version1_aot() {
  cd "$working_dir"
  cd csharp/version1
  bash build.sh aot >/dev/null
  hyperfine -i --output=pipe --runs 3 --warmup 2 -n "C# AOT" "bash run.sh aot"
}

all() {
  cd "$working_dir"
  csharp_version1
  csharp_version1_aot
}

if [ "$1" = "0" ]; then
  all
elif [ "$1" = "1" ]; then
  csharp_version1
elif [ "$1" = "2" ]; then
  csharp_version1_aot
else
  usage
fi
