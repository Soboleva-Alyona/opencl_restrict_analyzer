#!/bin/bash

cmake -S . -B build
cmake --build build

./cmake-build-debug/clsa \
  --input ./src/test.cl \
  --kernel test_violation_trivial \
  --work-dim 1 --global-work-size 32 --local-work-size 1 \
  --arg b16 --arg 10i64 \
  --check-restrict