#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "${BASH_SOURCE[0]}")/.."
mkdir -p build
flags=(-std=c++17 -Wall -Wextra -Wpedantic -Werror -g -Iinclude)
if [[ "${SANITIZE:-0}" == "1" ]]; then
    flags+=(-fsanitize=address,undefined -fno-omit-frame-pointer)
fi
sources=(src/backend/OperationEvent.cpp src/backend/BinaryTree.cpp src/backend/BST.cpp src/backend/AVLTree.cpp src/backend/MaxHeap.cpp)
if [[ "${1:-}" != "--campaign-only" ]]; then
for suite in tests/backend/test_*.cpp; do
    binary="build/$(basename "${suite%.cpp}")"
    g++ "${flags[@]}" "${sources[@]}" "$suite" -o "$binary"
    "$binary"
done
fi
middleware=(src/middleware/JsonDocument.cpp src/middleware/SaveFile.cpp src/middleware/EventMapper.cpp src/middleware/AnimationController.cpp src/middleware/BackendAdapter.cpp src/middleware/GameController.cpp src/middleware/Campaign.cpp src/middleware/AvlCampaign.cpp src/middleware/HeapCampaign.cpp)
if [[ "${1:-}" != "--campaign-only" ]]; then
g++ "${flags[@]}" "${sources[@]}" "${middleware[@]}" tests/middleware/test_endless.cpp -o build/test-endless
fi
testDirectory="$(mktemp -d build/endless-tests.XXXXXX)"
mkdir -p "$testDirectory/first" "$testDirectory/second"
if [[ "${1:-}" != "--campaign-only" ]]; then
build/test-endless "$testDirectory/first" "$testDirectory/second"
fi
mkdir -p "$testDirectory/campaign"
g++ "${flags[@]}" "${sources[@]}" "${middleware[@]}" tests/middleware/test_campaign.cpp -o build/test-campaign
build/test-campaign "$testDirectory/campaign"
echo "Campaign test saves: $testDirectory/campaign"
g++ "${flags[@]}" src/middleware/SaveFile.cpp tests/middleware/test_save_file.cpp -o build/test-save-file
build/test-save-file "$testDirectory/save-file.txt"
