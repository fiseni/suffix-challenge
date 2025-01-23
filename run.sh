#!/bin/bash

working_dir="$(pwd)"
echo ""

usage() {
  echo "Invalid argument!"
  echo "Usage: $0 <implementation number>"
  echo ""
  echo "Implementations:"
  echo "0: All"
  echo "1: C# v1"
  echo "2: C# v1 AOT"
  exit 1
}

csharp_v1() {
  cd "$working_dir"/csharp/v1
  bash build.sh >/dev/null
  hyperfine -i --output=pipe --runs 3 --warmup 2 --export-markdown "$working_dir/benchmarks/csharp_v1.md" -n "C# v1" "bash run.sh"
}

csharp_v1_aot() {
  cd "$working_dir"/csharp/v1
  bash build.sh aot >/dev/null
  hyperfine -i --output=pipe --runs 3 --warmup 2 --export-markdown "$working_dir/benchmarks/csharp_v1_aot.md" -n "C# v1 AOT" "bash run.sh aot"
}

all() {
  csharp_v1
  csharp_v1_aot
}

if [ "$1" = "0" ]; then
  all
elif [ "$1" = "1" ]; then
  csharp_v1
elif [ "$1" = "2" ]; then
  csharp_v1_aot
else
  usage
fi
