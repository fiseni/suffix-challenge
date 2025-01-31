#!/bin/bash

source "../../argparser.sh"

parts_file="../../data/parts.txt"
master_parts_file="../../data/master-parts.txt"
results_file="results.txt"

if [ $ARG_COUNT -ge 3 ]; then
  parts_file="$ARG1"
  master_parts_file="$ARG2"
  results_file="$ARG3"
fi

./publish/v1 $parts_file $master_parts_file $results_file
