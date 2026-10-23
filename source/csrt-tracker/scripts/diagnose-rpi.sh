#!/usr/bin/env bash
set -euo pipefail

echo "== Operating system =="
cat /etc/os-release
echo "== Architecture =="
uname -m
echo "== Memory =="
free -h
echo "== Root filesystem =="
df -h /
echo "== Temperature =="
vcgencmd measure_temp
echo "== Cameras =="
rpicam-hello --list-cameras
echo "== Package versions =="
dpkg-query -W \
    cmake qt6-base-dev qt6-wayland libopencv-dev \
    libgstreamer1.0-dev libgstreamer-plugins-base1.0-dev \
    gstreamer1.0-libcamera gstreamer1.0-plugins-ugly
echo "== Required GStreamer elements =="
for element in libcamerasrc glimagesink appsink x264enc; do
    gst-inspect-1.0 "$element" >/dev/null
    echo "$element: available"
done

echo "== Five-second IMX477 frame probe =="
probe_log="$(mktemp)"
trap 'rm -f "$probe_log"' EXIT
set +e
timeout 5s gst-launch-1.0 \
    libcamerasrc \
    sensor-config="sensor/config,width=2028,height=1080,depth=12" \
    ae-enable=true exposure-time-mode=manual exposure-time=5000 \
    analogue-gain-mode=auto ! \
    "video/x-raw,format=BGRx,width=2028,height=1080,framerate=50/1" ! \
    fakesink sync=false >"$probe_log" 2>&1
probe_status=$?
set -e
cat "$probe_log"

if [[ "$probe_status" -ne 124 ]]; then
    echo "Camera probe ended unexpectedly with status $probe_status." >&2
    exit 1
fi
if ! grep -Fq 'Setting pipeline to PLAYING' "$probe_log"; then
    echo "Camera probe never reached PLAYING." >&2
    exit 1
fi
if grep -Fq 'ERROR' "$probe_log"; then
    echo "Camera probe reported a GStreamer error." >&2
    exit 1
fi
echo "IMX477 frame probe passed."
