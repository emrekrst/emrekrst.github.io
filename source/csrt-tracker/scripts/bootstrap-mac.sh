#!/usr/bin/env bash
set -euo pipefail

if [[ "$(uname -s)" != "Darwin" ]]; then
    echo "This bootstrap script is for macOS."
    exit 1
fi

if ! command -v brew >/dev/null 2>&1; then
    echo "Homebrew is required: https://brew.sh"
    exit 1
fi

required=(qtbase gstreamer opencv@4 pkgconf cmake ninja)
missing=()
for package in "${required[@]}"; do
    if ! brew list --versions "$package" >/dev/null 2>&1; then
        missing+=("$package")
    fi
done

if (( ${#missing[@]} > 0 )); then
    brew install "${missing[@]}"
fi

project_root="$(cd "$(dirname "$0")/.." && pwd)"
cmake -S "$project_root" -B "$project_root/build" -G Ninja \
    -DCMAKE_BUILD_TYPE=Debug \
    "-DCMAKE_PREFIX_PATH=$(brew --prefix qtbase);$(brew --prefix opencv@4)"
cmake --build "$project_root/build"

echo "Built: $project_root/build/CameraTracker.app"
