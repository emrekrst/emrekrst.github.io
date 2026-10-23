#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
mkdir -p "$project_root/logs"

cmake -S "$project_root" -B "$project_root/build-rpi" -G Ninja \
    -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=ON
cmake --build "$project_root/build-rpi" --parallel
ctest --test-dir "$project_root/build-rpi" --output-on-failure --timeout 30
