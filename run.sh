#!/usr/bin/env bash

# Build and launch Vault Heist from WSL.
set -euo pipefail

project_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
build_dir="$project_dir/build"
executable="$build_dir/vault-heist"
sfml_prefix="/usr/local"

if ! command -v g++ >/dev/null 2>&1; then
    echo "g++ is required. In WSL, install it with: sudo apt update && sudo apt install build-essential"
    exit 1
fi

if [[ ! -f "$sfml_prefix/include/SFML/Graphics.hpp" ]]; then
    echo "SFML 2.6.1 was not found in $sfml_prefix. Install SFML 2.6.1 before running the project."
    exit 1
fi

mapfile -d '' sources < <(find "$project_dir/src" -type f -name '*.cpp' -print0 | sort -z)

if (( ${#sources[@]} == 0 )); then
    echo "No C++ source files found in $project_dir/src."
    exit 1
fi

mkdir -p "$build_dir"
mkdir -p "$build_dir/smoke-saves"

echo "Building Vault Heist..."
g++ -std=c++17 -Wall -Wextra -Wpedantic -g \
    -I"$project_dir/include" \
    -I"$sfml_prefix/include" \
    "${sources[@]}" \
    -L"$sfml_prefix/lib" \
    -Wl,-rpath,"$sfml_prefix/lib" \
    -lsfml-graphics -lsfml-window -lsfml-system \
    -o "$executable"

if [[ "${1:-}" == "--build-only" ]]; then
    exit 0
fi

echo "Starting Vault Heist..."
cd "$project_dir"
exec "$executable" "$@"
