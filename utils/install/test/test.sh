#!/bin/bash

# bail out whenever a command doesn't exit cleanly
set -e

VMEDESC=../vmedesc

# each .out file has the command to build it in its first line as a comment
for file in *.out
do
    $VMEDESC `sed 1q $file | sed 's/^# //'` | \
    diff -I'^#' --ignore-all-space --ignore-blank-lines $file -
done

echo "test OK"
