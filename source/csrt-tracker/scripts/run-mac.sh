#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "$0")/.." && pwd)"
app_path="$project_root/build/CameraTracker.app"

if [[ ! -d "$app_path" ]]; then
    "$project_root/scripts/bootstrap-mac.sh"
fi

cmake --build "$project_root/build" --target CameraTracker
open "$app_path"
