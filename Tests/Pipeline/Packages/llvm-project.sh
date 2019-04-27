#!/bin/bash

mkdir -p build
cd build
cmake -DLLVM_ENABLE_PROJECTS=clang -G "Unix Makefiles" ../llvm
make && sudo make install
