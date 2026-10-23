#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
camera_tracker_binary="${CAMERA_TRACKER_BINARY:-$project_root/build-rpi/CameraTracker}"
if [[ ! -x "$camera_tracker_binary" ]]; then
    echo "CameraTracker is not built. Run ./scripts/build-rpi.sh first." >&2
    exit 1
fi

export QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-wayland}"
export WAYLAND_DISPLAY="${WAYLAND_DISPLAY:-wayland-0}"
export XDG_RUNTIME_DIR="${XDG_RUNTIME_DIR:-/run/user/$(id -u)}"

cd "$project_root"
exec "$camera_tracker_binary" \
    --config "$project_root/config/camera-tracker.ini" "$@"
