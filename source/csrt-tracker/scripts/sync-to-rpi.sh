#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
rpi_target="${RPI_TARGET:?Set RPI_TARGET to user@host}"
remote_root="${RPI_REMOTE_ROOT:-CameraTracker}"

ssh "$rpi_target" mkdir -p "$remote_root"
rsync -aziv \
    --exclude-from="$project_root/scripts/rsync-excludes.txt" \
    "$project_root/" "$rpi_target:$remote_root/"
