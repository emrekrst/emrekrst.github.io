#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/.." && pwd)"
build="${1:-$root/build-host}"
python="${PYTHON:-python3}"
mkdir -p "$build"
"$python" -m unittest discover -s "$root/tests" -v
"${CXX:-c++}" -std=c++17 -Wall -Wextra -pedantic \
  -I"$root/tests/fake_sdk" -I"$root/final_navigation/firmware" \
  "$root/tests/firmware_test.cpp" -o "$build/firmware_test"
for scenario in protocol overflow zero center; do
  "$build/firmware_test" "$scenario"
done
