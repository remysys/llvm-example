#!/bin/bash

code=$(cat $1 | tr '\n' ' ')
echo $code
./build/clang-interpreter "$code"