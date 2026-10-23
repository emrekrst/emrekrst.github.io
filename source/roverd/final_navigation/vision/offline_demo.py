#!/usr/bin/env python3
"""Generated marker -> detector -> gate -> steering demonstration. No hardware I/O."""
import argparse
import json
from pathlib import Path

import cv2
import numpy as np

from config import IMG_WIDTH, IMG_HEIGHT, PATTERNS_PI
from custom_marker_finder import find_markers
from gate_finder import find_gates, best_gate
from path_finder import gate_to_command, x_to_letter


def synthetic_gate():
    frame = np.full((IMG_HEIGHT, IMG_WIDTH, 3), 235, np.uint8)
    for kind, x in [("left", 260), ("right", 860)]:
        patch = np.zeros((80, 80), np.uint8)
        for row in range(4):
            for col in range(4):
                patch[16+12*row:28+12*row, 16+12*col:28+12*col] = 0 if PATTERNS_PI[kind][row][col] else 255
        patch = cv2.resize(patch, (160, 160), interpolation=cv2.INTER_NEAREST)
        frame[260:420, x:x+160] = cv2.cvtColor(patch, cv2.COLOR_GRAY2BGR)
    return frame


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=Path("demo-output"))
    args = parser.parse_args()
    frame = synthetic_gate()
    markers = find_markers(frame=frame, include_unknown=False)
    gate = best_gate(find_gates(markers))
    command, _ = gate_to_command(gate)
    if len(markers) != 2 or gate is None or command != "go":
        raise RuntimeError("synthetic end-to-end detection failed")
    for marker in markers:
        corners = marker["corners"].astype(int)
        cv2.polylines(frame, [corners.reshape(-1, 1, 2)], True, (40, 160, 30), 3)
        cv2.putText(frame, marker["type"], tuple(corners[0] - [0, 12]), cv2.FONT_HERSHEY_SIMPLEX, .7, (20, 80, 20), 2)
    center = tuple(int(x) for x in gate["center"])
    cv2.circle(frame, center, 12, (30, 80, 220), -1)
    cv2.line(frame, tuple(map(int,gate["left"]["center"])), tuple(map(int,gate["right"]["center"])), (30, 80, 220), 3)
    letter = x_to_letter(gate["center"][0])
    cv2.putText(frame, f"OFFLINE / SYNTHETIC | steering: {command} | UART: {letter}", (45, 80), cv2.FONT_HERSHEY_SIMPLEX, .85, (30, 30, 30), 2)
    args.output.mkdir(parents=True, exist_ok=True)
    if not cv2.imwrite(str(args.output / "gate-demo.png"), frame):
        raise RuntimeError("could not save demo image")
    result = {"markers": len(markers), "center": [float(v) for v in gate["center"]], "command": command, "letter": letter, "hardware_used": False}
    (args.output / "result.json").write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps(result))


if __name__ == "__main__":
    main()
