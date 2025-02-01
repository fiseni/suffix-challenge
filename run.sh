#!/bin/bash
 
# Author: Fati Iseni
# Date: 2025-01-31

# The full path of this script.
script_dir=$(dirname "$(realpath "$0")")

# Parse positional arguments and options.
source $script_dir/argparser.sh

# Check if the implementation list file exists.
impl_list_file="$script_dir/impl_list"
if [ ! -f "$impl_list_file" ]; then
  echo "Error: Implementation list file '$impl_list_file' not found!"
  exit 1
fi

# If -t option is set, use test data files.
results_file_name="results.txt"
parts_file="$script_dir/data/parts.txt"
master_parts_file="$script_dir/data/master-parts.txt"
expected_file="$script_dir/data/expected.txt"

if [ -n "$OPT_T" ]; then
  parts_file="$script_dir/data/test-parts.txt"
  master_parts_file="$script_dir/data/test-master-parts.txt"
  expected_file="$script_dir/data/test-expected.txt"
fi

# If -s or -t options are set, do not run benchmarks.
is_simple_run="false"
if [ -n "$OPT_S" ] || [ -n "$OPT_T" ]; then
  is_simple_run="true"
fi

# If -s or -t options are set, do not run benchmarks.
nobuild="false"
if [ -n "$OPT_B" ]; then
  nobuild="true"
fi

####################################################################
show_help() {
  echo ""
  echo "Usage: $0 <implementation number>"
  echo ""
  echo "Implementations:"
  while IFS=":" read -r num desc _; do
    echo "$num: $desc"
  done < <(grep -v "^#\|^[[:space:]]*$" $impl_list_file)
  echo ""
  exit 1
}

####################################################################
run_all() {
  while IFS=":" read -r num _; do
    num=$(echo "$num" | xargs)
    if [ "$num" == "0" ]; then
      continue
    fi
    run_impl $num
  done < <(grep -v "^#\|^[[:space:]]*$" $impl_list_file)
}

####################################################################
run_impl() {
  # If any command fails, exit immediately.
  set -euo pipefail

  found=false
  while IFS=":" read -r num desc langdir subdir tag _; do
    if [ "$1" = "$num" ]; then

      # We will switch to impl dir just in case if someone assumes that in impl scripts.
      echo ""
      found=true
      impl_dir="$script_dir/$langdir/$subdir"
      cd $impl_dir

      # We will export hyperfine results as md files in this directory.
      mkdir -p "$script_dir/benchmarks"

      # Always use absolute paths, just in case.
      if [ "$nobuild" == "false" ]; then
        echo "Building \"$desc\" implementation..."
        bash $impl_dir/build.sh $tag >/dev/null
        echo "Build completed."
        echo ""
      fi
      
      # The output results file is always in the impl directory.
      results_file="$impl_dir/$results_file_name"
      rm -f $results_file >/dev/null 2>&1

      # If -s or -t options are set, we will just execute the script.
      if [ "$is_simple_run" == "true" ]; then
        bash $impl_dir/run.sh $parts_file $master_parts_file $results_file $tag
      else
        cmd="bash $impl_dir/run.sh $parts_file $master_parts_file $results_file"
        hyperfine -i --runs 10 --warmup 3 --export-markdown "$script_dir/benchmarks/${num}.md" -n "\"$desc\"" "$cmd"
      fi
      
      # Check for the correctness of results.
      compare_results $expected_file $results_file
      echo "-----------------------------------------------------------------------------------------------------------------"
      break
    fi
  done < "$impl_list_file"
  if [ "$found" = false ]; then
    show_help
  fi
}

####################################################################
compare_results() {
  NC='\033[0m'
  RED='\033[0;31m'
  GREEN='\033[0;32m'
  set +e +u +o pipefail
  if [ ! -f "$2" ]; then
    echo "File $2 not found!"
  else
    diff --strip-trailing-cr -y --suppress-common-lines "$1" "$2" > temp_diff.txt
    if [ "$?" == 0 ]; then
      echo -e "${GREEN}Test passed!${NC}"
    else
      echo -e "${RED}Test failed${NC}. The produced result file is different than the expected file. Here are the first 5 different lines."
      echo ""
      head -n 5 temp_diff.txt
    fi
    rm temp_diff.txt >/dev/null 2>&1
  fi
  echo ""
}

####################################################################
if [ -n "$OPT_H" ]; then
  show_help
elif [ -z "$ARG1" ]; then
  show_help
elif [ "$ARG1" = "0" ]; then
  run_all
else
  run_impl $ARG1
fi
