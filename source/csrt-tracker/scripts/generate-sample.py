#!/usr/bin/env python3
"""Generate the redistributable synthetic recorded-video fixture; no camera access."""
from pathlib import Path
import cv2
import numpy as np

path = Path(__file__).resolve().parents[1] / "samples/moving-target.avi"
path.parent.mkdir(exist_ok=True)
writer = cv2.VideoWriter(str(path), cv2.VideoWriter_fourcc(*"MJPG"), 10, (640, 360))
if not writer.isOpened():
    raise RuntimeError("MJPG VideoWriter unavailable")
for index in range(60):
    frame = np.full((360, 640, 3), (130, 145, 160), dtype=np.uint8)
    x = 80 + 2 * index
    cv2.rectangle(frame, (x, 120), (x + 47, 147), (35, 75, 205), -1)
    cv2.rectangle(frame, (x, 120), (x + 47, 147), (10, 20, 40), 2)
    cv2.circle(frame, (x + 24, 134), 4, (50, 90, 220), -1)
    writer.write(frame)
writer.release()
print(path)

# File-backed selector regressions use generated imagery, not private video crops.
fixtures = path.parents[1] / "tests/fixtures"
fixtures.mkdir(exist_ok=True)
road = np.full((320, 320, 3), (118, 122, 126), dtype=np.uint8)
cv2.rectangle(road, (100, 100), (220, 220), (76, 79, 82), 3)
car = np.full((320, 320, 3), (140, 145, 150), dtype=np.uint8)
cv2.rectangle(car, (134, 146), (188, 174), (35, 70, 200), -1)
cv2.rectangle(car, (134, 146), (188, 174), (10, 20, 40), 2)
cv2.rectangle(car, (145, 149), (177, 154), (60, 75, 90), -1)
for name, frame in [("road-click.png", road), ("car-click.png", car)]:
    if not cv2.imwrite(str(fixtures / name), frame):
        raise RuntimeError(f"could not write {name}")
