#!/usr/bin/env bash
set -euo pipefail

QT_PREFIX="$(brew --prefix qt)"
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH="$QT_PREFIX"
cmake --build build

echo "Built: build/ProsToggleUploader.app"
