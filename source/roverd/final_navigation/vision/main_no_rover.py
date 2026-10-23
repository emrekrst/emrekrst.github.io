# main_no_rover.py - Vision-only pipeline for Raspberry Pi
# Camera -> Squares -> Pattern -> Gate -> Command -> Web stream

import threading
import time
import numpy as np
import cv2
from http.server import BaseHTTPRequestHandler

from config import (IMG_WIDTH, IMG_HEIGHT, FRAMERATE,
                    CAMERA_METADATA_LOG_INTERVAL, STREAM_JPEG_QUALITY,
                    STREAM_OVERLAY,
                    LED_PINS, SERIAL_TX_PIN, SERIAL_PORT, SERIAL_BAUD_RATE, LOG_RUNTIME,
                    GATE_PRESENT_THRESHOLD, GATE_ABSENT_THRESHOLD,
                    COOLDOWN_FRAMES, NO_GATE_CHAR, STRAIGHT_CHAR)
from camera_utils import capture_bgr_frame, camera_metadata_summary, open_camera
from custom_marker_finder import find_markers
from gate_finder import find_gates, best_gate
from gate_tracker import GateTracker
from path_finder import gate_to_command, x_to_letter
from stream_server import create_stream_server, stream_url

# ===== HARDWARE OUTPUTS DISABLED =====
print("[NO ROVER] GPIO and UART disabled; running camera/detection/web stream only.")

print(f"[NO ROVER] Would use LEDs={LED_PINS} and UART port={SERIAL_PORT} TX=GPIO{SERIAL_TX_PIN} baud={SERIAL_BAUD_RATE}")

# ===== WEB STREAM =====
PORT = 8080
latest_frame = None
frame_lock = threading.Lock()


class MJPEGHandler(BaseHTTPRequestHandler):
    def do_GET(self):
        if self.path == '/':
            self.send_response(200)
            self.send_header('Content-type', 'text/html')
            self.end_headers()
            self.wfile.write(
                b'<html><body style="margin:0;background:#000">'
                b'<img src="/stream" style="width:100%;height:100vh;object-fit:contain">'
                b'</body></html>'
            )
        elif self.path == '/stream':
            self.send_response(200)
            self.send_header('Cache-Control', 'no-cache')
            self.send_header('Content-Type', 'multipart/x-mixed-replace; boundary=FRAME')
            self.end_headers()
            try:
                while True:
                    with frame_lock:
                        frame = latest_frame
                    if frame is None:
                        time.sleep(0.05)
                        continue
                    ret, jpg = cv2.imencode(
                        '.jpg', frame,
                        [cv2.IMWRITE_JPEG_QUALITY, STREAM_JPEG_QUALITY]
                    )
                    if not ret:
                        continue
                    self.wfile.write(b'--FRAME\r\nContent-Type: image/jpeg\r\n\r\n')
                    self.wfile.write(jpg.tobytes())
                    self.wfile.write(b'\r\n')
                    time.sleep(1 / FRAMERATE)
            except Exception:
                pass

    def log_message(self, _format, *_args):
        pass


def set_leds(command):
    """No-op in Pi-only vision test mode."""
    return


# ===== SERIAL COMMAND RATE CONTROL =====
_CMD_RATE_HZ  = 25                    # target TX frequency (Hz)
_CMD_INTERVAL = 1.0 / _CMD_RATE_HZ   # minimum seconds between writes
_last_cmd_time  = 0.0
_first_tx_done  = False


def send_serial_letter(letter):
    """Log the first TX only; no UART is used in Pi-only vision test mode."""
    global _last_cmd_time, _first_tx_done
    now = time.monotonic()
    if now - _last_cmd_time < _CMD_INTERVAL:
        return
    _last_cmd_time = now
    if not _first_tx_done:
        print(f"[NO ROVER] First simulated TX: '{letter}'  t={now:.3f}s")
        _first_tx_done = True


def draw_overlay(frame, markers, gate, command, letter='?', gate_num=0, state=''):
    """Draw detection results onto the frame for the web stream."""
    vis = frame.copy()

    # Draw all detected markers
    for m in markers:
        corners = m['corners'].astype(int)
        color = (0, 255, 0) if m['type'] == 'left' else \
                (0, 0, 255) if m['type'] == 'right' else (128, 128, 128)
        cv2.polylines(vis, [corners.reshape(-1, 1, 2)], True, color, 2)
        cx, cy = int(m['center'][0]), int(m['center'][1])
        cv2.putText(vis, m['type'], (cx - 20, cy - 10),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.5, color, 2)

    # Draw gate center and width
    if gate:
        gate_color = (0, 180, 255) if gate.get('stale') else (0, 255, 255)
        gx, gy = int(gate['center'][0]), int(gate['center'][1])
        cv2.circle(vis, (gx, gy), 10, gate_color, -1)
        cv2.line(vis, (int(gate['left']['center'][0]), int(gate['left']['center'][1])),
                      (int(gate['right']['center'][0]), int(gate['right']['center'][1])),
                      gate_color, 2)
        if gate.get('stale'):
            cv2.putText(vis, "gate hold", (gx + 12, gy),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.5, gate_color, 1)

    # Draw command
    cmd_text = command if command else 'no gate'
    cv2.putText(vis, cmd_text, (10, 30),
                cv2.FONT_HERSHEY_SIMPLEX, 0.8, (255, 255, 255), 2)

    # Draw serial letter, gate number, and state
    case_label = "UPPER" if (gate_num % 2 == 0) else "lower"
    info_text = f"TX:{letter}  Gate#{gate_num+1}({case_label})  [{state}]"
    cv2.putText(vis, info_text, (10, 60),
                cv2.FONT_HERSHEY_SIMPLEX, 0.6, (0, 200, 255), 2)

    return vis


# ===== START WEB SERVER =====
server, PORT = create_stream_server(MJPEGHandler, PORT)
server_thread = threading.Thread(target=server.serve_forever)
server_thread.daemon = True
server_thread.start()
print(f"Stream: {stream_url(PORT)}")

# ===== START CAMERA =====
picam2 = open_camera()
print("Running pipeline...")

# ===== GATE CROSSING STATE =====
gate_counter = 0                # How many gates the rover has passed
frames_gate_present = 0         # Consecutive frames with a gate detected
frames_gate_absent = 0          # Consecutive frames with no gate detected
gate_was_solidly_present = False # True once gate visible >= GATE_PRESENT_THRESHOLD
clearing_frames_left = 0        # Countdown during CLEARING phase
last_letter = NO_GATE_CHAR      # Last letter sent
gate_tracker = GateTracker()
frame_count = 0

try:
    while True:
        t0 = time.time()
        frame_count += 1

        # 1. Capture frame
        frame = capture_bgr_frame(picam2)
        if (LOG_RUNTIME and CAMERA_METADATA_LOG_INTERVAL and
                frame_count % CAMERA_METADATA_LOG_INTERVAL == 0):
            print(f"[camera] {camera_metadata_summary(picam2)}")
        gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)

        # 2. Detect custom grid markers
        markers = find_markers(gray=gray, frame=frame)

        # 3. Find and smooth valid gates
        gates = find_gates(markers)
        raw_gate = best_gate(gates)
        gate = gate_tracker.update(raw_gate)

        # 4. Get steering command (for LEDs - unchanged)
        command, led_pin = gate_to_command(gate)

        # 5. Gate crossing state machine
        use_uppercase = (gate_counter % 2 == 0)

        if clearing_frames_left > 0:
            # CLEARING phase: go straight, ignore any gates
            state = 'CLEARING'
            letter = STRAIGHT_CHAR if use_uppercase else STRAIGHT_CHAR.lower()
            clearing_frames_left -= 1
            if clearing_frames_left == 0:
                # PASSED: done clearing, toggle case for next gate
                gate_counter += 1
                gate_was_solidly_present = False
                frames_gate_present = 0
                frames_gate_absent = 0
                last_letter = NO_GATE_CHAR
                gate_tracker.reset()
                if LOG_RUNTIME:
                    print(f"*** GATE {gate_counter} PASSED ***")

        elif gate is not None:
            # Gate is visible
            frames_gate_present += 1
            frames_gate_absent = 0
            if frames_gate_present >= GATE_PRESENT_THRESHOLD:
                gate_was_solidly_present = True

            letter = x_to_letter(gate['center'][0], uppercase=use_uppercase)
            last_letter = letter
            state = 'TRACKING'

        else:
            # No gate visible
            frames_gate_present = 0
            frames_gate_absent += 1

            if gate_was_solidly_present and frames_gate_absent >= GATE_ABSENT_THRESHOLD:
                # Gate was solidly visible, now gone long enough -> start clearing
                clearing_frames_left = COOLDOWN_FRAMES
                letter = STRAIGHT_CHAR if use_uppercase else STRAIGHT_CHAR.lower()
                state = 'CLEARING'
            elif gate_was_solidly_present:
                # Brief dropout - keep sending last known letter
                letter = last_letter
                state = 'DROPOUT'
            else:
                # Never saw a gate solidly
                letter = NO_GATE_CHAR
                state = 'SEARCHING'

        # 6. Output: LEDs + serial
        set_leds(command)
        send_serial_letter(letter)

        # 7. Update web stream
        if STREAM_OVERLAY:
            vis = draw_overlay(frame, markers, gate, command,
                               letter=letter, gate_num=gate_counter, state=state)
        else:
            vis = frame
        with frame_lock:
            latest_frame = vis

        if LOG_RUNTIME:
            print(f"{1/(time.time()-t0):.1f} fps | markers={len(markers)} | "
                  f"raw_gate={'yes' if raw_gate else 'no'} | "
                  f"gate={'hold' if gate and gate.get('stale') else 'yes' if gate else 'no'} | "
                  f"cmd={command} | "
                  f"TX={letter} | gate#{gate_counter+1} | {state}")

except KeyboardInterrupt:
    print("\nStopped.")
finally:
    picam2.stop()
    server.shutdown()
    print("Done.")
