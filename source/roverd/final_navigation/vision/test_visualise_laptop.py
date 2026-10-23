# test_visualise_laptop.py - Test full pipeline on Mac webcam
# Shows: detected squares, marker labels, gate, steering zones, command

import cv2
import time

import config
from config import IMG_WIDTH, IMG_HEIGHT, STEER_ZONE_LEFT, STEER_ZONE_RIGHT

try:
    from config_mac import PATTERNS_MAC
    config.PATTERNS = PATTERNS_MAC
    PATTERN_SOURCE = 'config_mac.py'
except ImportError:
    PATTERN_SOURCE = 'config.py'

from custom_marker_finder import find_markers
from gate_finder import find_gates, best_gate
from gate_tracker import GateTracker
from path_finder import gate_to_command


def draw_overlay(frame, markers, gate, command):
    vis = frame.copy()
    h, w = vis.shape[:2]

    # --- Steering zone lines ---
    zone_left = int(STEER_ZONE_LEFT * w / IMG_WIDTH)
    zone_right = int(STEER_ZONE_RIGHT * w / IMG_WIDTH)
    mid = w // 2

    cv2.line(vis, (zone_left, 0), (zone_left, h), (50, 50, 50), 1)
    cv2.line(vis, (zone_right, 0), (zone_right, h), (50, 50, 50), 1)
    cv2.line(vis, (mid, 0), (mid, h), (30, 30, 30), 1)

    # Zone labels
    cv2.putText(vis, 'SL', (20, h - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.4, (100, 100, 100), 1)
    cv2.putText(vis, 'LL', (zone_left + 5, h - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.4, (100, 100, 100), 1)
    cv2.putText(vis, 'GO', (mid - 15, h - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.4, (100, 100, 100), 1)
    cv2.putText(vis, 'LR', (zone_right + 5, h - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.4, (100, 100, 100), 1)
    cv2.putText(vis, 'SR', (w - 30, h - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.4, (100, 100, 100), 1)

    # --- Detected markers ---
    for m in markers:
        corners = m['corners'].astype(int)
        if m['type'] == 'left':
            color = (0, 255, 0)    # green
        elif m['type'] == 'right':
            color = (0, 0, 255)    # red
        else:
            color = (128, 128, 128)  # grey = unknown

        cv2.polylines(vis, [corners.reshape(-1, 1, 2)], True, color, 2)
        cx, cy = int(m['center'][0]), int(m['center'][1])
        cv2.circle(vis, (cx, cy), 4, color, -1)
        cv2.putText(vis, f"{m['type']} d={m.get('match_error', '?')}", (cx - 35, cy - 12),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.5, color, 2)

    # --- Gate ---
    if gate:
        gate_color = (0, 180, 255) if gate.get('stale') else (0, 255, 255)
        lx, ly = int(gate['left']['center'][0]), int(gate['left']['center'][1])
        rx, ry = int(gate['right']['center'][0]), int(gate['right']['center'][1])
        gx, gy = int(gate['center'][0]), int(gate['center'][1])

        # Line between markers
        cv2.line(vis, (lx, ly), (rx, ry), gate_color, 2)
        # Gate center
        cv2.circle(vis, (gx, gy), 10, gate_color, -1)
        cv2.putText(vis, "gate hold" if gate.get('stale') else "gate center", (gx + 12, gy),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.4, gate_color, 1)

    # --- Command ---
    cmd_text = command.upper() if command else 'NO GATE'
    cmd_color = (0, 255, 0) if command == 'go' else \
                (0, 165, 255) if command and 'left' in command else \
                (255, 100, 100) if command and 'right' in command else \
                (100, 100, 100)
    cv2.putText(vis, cmd_text, (10, 35),
                cv2.FONT_HERSHEY_SIMPLEX, 1.0, cmd_color, 3)

    types = [m.get('type') for m in markers]
    cv2.putText(vis, f"custom={types} patterns={PATTERN_SOURCE}", (10, 68),
                cv2.FONT_HERSHEY_SIMPLEX, 0.6, (255, 255, 255), 2)

    return vis


print("=" * 50)
print("LAPTOP VISUALISER (Mac Webcam)")
print("=" * 50)
print("Press Q to quit")
print(f"Using custom grid markers from {PATTERN_SOURCE}")
print("=" * 50)

cap = cv2.VideoCapture(0, cv2.CAP_AVFOUNDATION)
if not cap.isOpened():
    print("ERROR: Cannot open webcam")
    print("macOS is still blocking camera access for the app running this script.")
    print("Quit/reopen that app after enabling Camera permission, then run this again.")
    exit(1)

cap.set(cv2.CAP_PROP_FRAME_WIDTH, IMG_WIDTH)
cap.set(cv2.CAP_PROP_FRAME_HEIGHT, IMG_HEIGHT)
cap.set(cv2.CAP_PROP_FPS, 30)
gate_tracker = GateTracker()

try:
    while True:
        t0 = time.time()

        ok, frame = cap.read()
        if not ok:
            break

        gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
        markers = find_markers(gray=gray, frame=frame)

        gates = find_gates(markers)
        raw_gate = best_gate(gates)
        gate = gate_tracker.update(raw_gate)
        command, _ = gate_to_command(gate)

        vis = draw_overlay(frame, markers, gate, command)
        cv2.imshow("Laptop Test - Full Pipeline", vis)

        fps = 1 / (time.time() - t0)
        details = [(m['type'], m.get('match_error'), m.get('grid').tolist()) for m in markers]
        print(f"{fps:.1f} fps | markers={len(markers)} | custom={details} | "
              f"raw_gate={'yes' if raw_gate else 'no'} | "
              f"gate={'hold' if gate and gate.get('stale') else 'yes' if gate else 'no'} | "
              f"cmd={command}")

        if cv2.waitKey(1) & 0xFF == ord('q'):
            break

except KeyboardInterrupt:
    print("\nStopped.")
finally:
    cap.release()
    cv2.destroyAllWindows()
    print("Done.")
