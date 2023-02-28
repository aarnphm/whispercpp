#!/bin/bash

set -xe

# Setup clang
if [[ $RUNNER_OS == "Linux" ]]; then
    export CC=gcc
    export CXX=g++
elif [[ $RUNNER_OS == "macOS" ]]; then
    brew install llvm
    export CC=clang
    export CXX=clang++
fi
