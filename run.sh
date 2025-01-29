#!/bin/bash
 
script_dir=$(dirname "$(realpath "$0")")
impl_list_file="$script_dir/impl_list"

if [ ! -f "$impl_list_file" ]; then
  echo "Error: Implementation list file '$impl_list_file' not found!"
  exit 1
fi

usage() {
  echo ""
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
      rm -f "result.txt" >/dev/null 2>&1
      echo ""
      if [ "$tag" = "" ]; then
        hyperfine -i --runs 10 --warmup 3 --export-markdown "$script_dir/benchmarks/${langdir}_${subdir}.md" -n "\"$desc\"" "\"./run.sh\""
      else
        hyperfine -i --runs 10 --warmup 3 --export-markdown "$script_dir/benchmarks/${langdir}_${subdir}_${tag}.md" -n "\"$desc\"" "./run.sh $tag"
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
    diff --strip-trailing-cr -y --suppress-common-lines "$1" "$2" > temp_diff.txt
    if [ "$?" == 0 ]; then
      echo "Test passed!"
    else
      echo "Test failed. The produced result file is different than the expected file. Here are the first 5 different lines."
      echo ""
      head -n 5 temp_diff.txt
    fi
    rm temp_diff.txt >/dev/null 2>&1
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
