#!/bin/bash
 
script_dir=$(dirname "$(realpath "$0")")
impl_list_file="$script_dir/impl_list"

if [ ! -f "$impl_list_file" ]; then
  echo "Error: Implementation list file '$impl_list_file' not found!"
  exit 1
fi

usage() {
  echo ""
  echo "Invalid argument!"
  echo "Usage: $0 <implementation number>"
  echo ""
  echo "Implementations:"
  while IFS=":" read -r num desc _; do
    echo "$num: $desc"
  done < "$impl_list_file"
  echo ""
  exit 1
}

run_all() {
  while IFS=":" read -r num _; do
    if [ "$num" != "0" ]; then
      run_impl $num
    fi
  done < "$impl_list_file"
}

run_impl() {
  set -euo pipefail
  found=false
  while IFS=":" read -r num desc langdir subdir tag _; do
    if [ "$1" = "$num" ]; then
      mkdir -p "$script_dir/benchmarks"
      echo ""
      found=true
      cd "$script_dir/$langdir/$subdir"
      echo "Building \"$desc\" implementation..."
      bash build.sh $tag >/dev/null
      echo "Build completed."
      echo ""
      if [ "$tag" = "" ]; then
        hyperfine -i --runs 3 --warmup 2 --export-markdown "$script_dir/benchmarks/${langdir}_${subdir}.md" -n "\"$desc\"" "\"./run.sh\""
      else
        hyperfine -i --runs 3 --warmup 2 --export-markdown "$script_dir/benchmarks/${langdir}_${subdir}_${tag}.md" -n "\"$desc\"" "./run.sh $tag"
      fi

      compare_results "$script_dir/data/expected.txt" "results.txt"
      echo "-----------------------------------------------------------------------------------------------------------------"
      break
    fi
  done < "$impl_list_file"
  if [ "$found" = false ]; then
    usage
  fi
}

compare_results() {
  set +e +u +o pipefail
  if [ ! -f "$2" ]; then
    echo "File $2 not found!"
  else
    diff --strip-trailing-cr -y --suppress-common-lines "$1" "$2" >/dev/null 2>&1
    if [ "$?" == 0 ]; then
      echo "Test passed!"
    else
      echo "Test failed!"
    fi
  fi
  echo ""
}

# Validate input
if [ -z "$1" ]; then
  usage
elif [ "$1" = "0" ]; then
  run_all
else
  run_impl $1
fi
