#!/bin/bash

### Check dependencies

# clang-format
if ! command -v clang-format &> /dev/null
then
    sudo apt install -y clang-format
fi

# clang-tidy
if ! command -v clang-tidy &> /dev/null
then
    sudo apt install -y clang-tidy
fi

# cmake-format
if ! command -v cmake-format &> /dev/null
then
    pip install cmakelang
fi

# lcov
if ! command -v lcov &> /dev/null
then
    sudo apt install lcov
fi

### Build

# Clean build
rm -rf build_local

mkdir -p build_local
cd build_local
cmake -DLOCAL_BUILD=ON -DTOOLS=all -DPROJECTS=all -DTESTS=all ..
make
