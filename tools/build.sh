#!/usr/bin/env bash
# -DCMAKE_CXX_COMPILER=g++-9
# -DENABLE_BUILD_TESTS=ON
cmake -B build -DENABLE_BUILD_TESTS=OFF
cmake --build build -j $(nproc)