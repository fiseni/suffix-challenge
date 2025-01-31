#!/bin/bash

# Author: Fati Iseni
# Date: 2025-01-31
# Script to parse both positional arguments and options combined.
# Options can be placed anywhere while calling the script.
# You can include the script in your bash file as follows.
# source argparser.sh

script_args=()
while [ $OPTIND -le "$#" ]
do
  if getopts :hsta option
  then
    case $option in
      h) OPT_H="true";;
      s) OPT_S="true";;
      t) OPT_T="true";;
      a) OPT_A="true";;
      \?) echo "Error: Invalid option"
          exit 1;;
    esac
  else
    script_args+=("${!OPTIND}")
    ((OPTIND++))
  fi
done

ARG_COUNT="${#script_args[@]}"

# Dynamically create ARG1, ARG2, ARG3, ...
for i in "${!script_args[@]}"
do
  eval "ARG$((i+1))=\"${script_args[$i]}\""
done
