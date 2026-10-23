#!/usr/bin/env bash
set -euo pipefail

if [[ "$(uname -m)" != "aarch64" ]]; then
    echo "This setup supports Raspberry Pi OS aarch64 only." >&2
    exit 1
fi
if [[ ! -f /etc/rpi-issue ]]; then
    echo "This does not appear to be Raspberry Pi OS." >&2
    exit 1
fi

# shellcheck disable=SC1091
source /etc/os-release
if [[ "${VERSION_CODENAME:-}" != "trixie" ]]; then
    echo "Expected Raspberry Pi OS Trixie; found ${PRETTY_NAME:-unknown}." >&2
    exit 1
fi

camera_list="$(rpicam-hello --list-cameras 2>&1)"
if ! grep -qi 'imx477' <<<"$camera_list"; then
    echo "IMX477 was not found by rpicam-hello." >&2
    exit 1
fi

packages=(
    cmake
    ninja-build
    pkg-config
    qt6-base-dev
    qt6-wayland
    libopencv-dev
    libgstreamer1.0-dev
    libgstreamer-plugins-base1.0-dev
    gstreamer1.0-libcamera
    gstreamer1.0-plugins-ugly
    rsync
)
missing=()
for package in "${packages[@]}"; do
    if ! dpkg-query -W -f='${Status}' "$package" 2>/dev/null \
        | grep -q 'install ok installed'; then
        missing+=("$package")
    fi
done

if (( ${#missing[@]} > 0 )); then
    sudo apt-get update
    sudo apt-get install -y "${missing[@]}"
else
    echo "Required packages are already installed."
fi

for element in libcamerasrc glimagesink appsink x264enc; do
    gst-inspect-1.0 "$element" >/dev/null
done

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
mkdir -p "$project_root/logs"
echo "Raspberry Pi setup checks passed."
