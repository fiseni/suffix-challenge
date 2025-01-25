#!/bin/bash

rm -rf publish
mkdir publish

FLAGS="-O3 -march=native -s -DNDEBUG -Wall -Wextra -Wno-unused-function"
FILES="main.c utils.c cross_platform_time.c thread_utils.c hash_table_string.c hash_table_sizelist.c source_data.c processor.c"

gcc $FLAGS $FILES -o publish/v1
