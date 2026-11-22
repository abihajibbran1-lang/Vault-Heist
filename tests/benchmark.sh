#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."
mkdir -p build docs/benchmarks
g++ -std=c++17 -O2 -Wall -Wextra -Wpedantic -Werror -Iinclude \
    src/backend/OperationEvent.cpp src/backend/BinaryTree.cpp src/backend/BST.cpp \
    src/backend/AVLTree.cpp src/backend/MaxHeap.cpp tests/benchmark.cpp -o build/benchmark
build/benchmark docs/benchmarks
