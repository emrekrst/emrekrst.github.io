#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
temp_root="$(mktemp -d)"
trap 'rm -rf "$temp_root"' EXIT
fake_bin="$temp_root/bin"
command_log="$temp_root/commands.log"
mkdir -p "$fake_bin"

for script in setup-rpi.sh build-rpi.sh run-rpi.sh diagnose-rpi.sh \
    sync-to-rpi.sh sync-from-rpi.sh; do
    bash -n "$project_root/scripts/$script"
done

cat >"$fake_bin/ssh" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail
printf 'ssh:%s\n' "$*" >>"$COMMAND_LOG"
if [[ "${FAIL_SSH:-0}" == "1" ]]; then
    exit 42
fi
EOF

cat >"$fake_bin/rsync" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail
printf 'rsync:%s\n' "$*" >>"$COMMAND_LOG"
EOF

cat >"$fake_bin/CameraTracker" <<'EOF'
#!/usr/bin/env bash
set -euo pipefail
printf 'camera:platform=%s wayland=%s runtime=%s args=%s\n' \
    "${QT_QPA_PLATFORM:-}" "${WAYLAND_DISPLAY:-}" "${XDG_RUNTIME_DIR:-}" \
    "$*" >>"$COMMAND_LOG"
EOF
chmod +x "$fake_bin/ssh" "$fake_bin/rsync" "$fake_bin/CameraTracker"

export PATH="$fake_bin:$PATH"
export COMMAND_LOG="$command_log"
export RPI_TARGET="pi-test"
export RPI_REMOTE_ROOT="CameraTracker"

"$project_root/scripts/sync-to-rpi.sh"
grep -F 'ssh:pi-test mkdir -p CameraTracker' "$command_log"
grep -F 'rsync:' "$command_log" | grep -F -- '--exclude-from=' | grep -F 'CameraTracker/'
if grep -F -- '--delete' "$command_log"; then
    echo 'sync-to-rpi.sh must not delete remote files' >&2
    exit 1
fi

: >"$command_log"
"$project_root/scripts/sync-from-rpi.sh"
grep -F 'ssh:pi-test test -d CameraTracker' "$command_log"
grep -F 'rsync:' "$command_log" | grep -F 'pi-test:CameraTracker/'
if grep -F -- '--delete' "$command_log"; then
    echo 'sync-from-rpi.sh must not delete local files' >&2
    exit 1
fi

: >"$command_log"
set +e
FAIL_SSH=1 "$project_root/scripts/sync-to-rpi.sh"
failure_status=$?
set -e
[[ "$failure_status" -eq 42 ]]
if grep -F 'rsync:' "$command_log"; then
    echo 'sync continued after SSH failed' >&2
    exit 1
fi

: >"$command_log"
(
    unset QT_QPA_PLATFORM WAYLAND_DISPLAY XDG_RUNTIME_DIR
    CAMERA_TRACKER_BINARY="$fake_bin/CameraTracker" \
        "$project_root/scripts/run-rpi.sh" --smoke-test
)
grep -F "camera:platform=wayland wayland=wayland-0 runtime=/run/user/$(id -u) args=--config $project_root/config/camera-tracker.ini --smoke-test" \
    "$command_log"
