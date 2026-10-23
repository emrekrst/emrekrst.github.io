# config.py - Master Settings File
# Add more settings as you build other files!

# ===== IMAGE SETTINGS =====
IMG_WIDTH = 1280
IMG_HEIGHT = 720
FRAMERATE = 25
# Picamera2's libcamera format names are counter-intuitive here: RGB888 gives
# numpy arrays in OpenCV-friendly BGR channel order.
CAMERA_FORMAT = "RGB888"
CAMERA_RAW_WIDTH = 1296
CAMERA_RAW_HEIGHT = 972
CAMERA_BUFFER_COUNT = 4
CAMERA_STARTUP_RETRIES = 3
CAMERA_FRAME_DURATION_LIMITS = (33333, 66666)
CAMERA_AE_ENABLE = True
CAMERA_AWB_ENABLE = True
CAMERA_NOISE_REDUCTION_MODE = 2
CAMERA_METADATA_LOG_INTERVAL = 50
STREAM_JPEG_QUALITY = 85

# ===== EDGE DETECTION SETTINGS =====
GAUSSIAN_KERNEL_SIZE = 5
GAUSSIAN_SIGMA = 1.4
CANNY_LOW_THRESHOLD = 50
CANNY_HIGH_THRESHOLD = 150

# ===== MARKER PATTERNS (4x4 grid: 1=black, 0=white) =====
# Pi camera calibrated patterns
PATTERNS_PI = {
    'left': [[0, 1, 0, 1],
             [0, 0, 0, 0],
             [0, 1, 1, 1],
             [0, 0, 0, 0]],

    'right': [[1, 0, 1, 0],
              [1, 1, 0, 0],
              [1, 1, 0, 1],
              [0, 0, 1, 1]]
}

# Mac webcam calibrated patterns (import from config_mac.py if it exists)
try:
    from config_mac import PATTERNS_MAC
except ImportError:
    PATTERNS_MAC = None

# Active patterns - defaults to Pi, override in platform-specific scripts
PATTERNS = PATTERNS_PI

# ===== CUSTOM GRID MARKERS =====
# Used by custom_marker_finder.py. A small Hamming-distance allowance makes the
# old 4x4 markers more tolerant of blur and lighting while still rejecting
# badly read grids as unknown.
CUSTOM_MARKER_MAX_HAMMING = 2
CUSTOM_MARKER_DUPLICATE_CENTER_FRACTION = 0.35

# ===== ARUCO MARKERS =====
# Marker ids printed on the physical gate posts. The ArUco detector maps
# these ids into the existing 'left'/'right' marker types used downstream.
# Current physical marker set. identify_aruco_markers.py detected the shown
# arrow marker as DICT_4X4_250 id=190. DICT_4X4_1000 may also report it
# because it is a larger 4x4 dictionary, but 250 is the tighter match.
ARUCO_DICTIONARY = 'DICT_4X4_250'
ARUCO_MARKER_IDS = {
    'left': [190],
    'right': [],  # Fill this after identifying the right-side marker ID.
}

# ===== PATH FINDER RULES =====
AREA_MATCH_THRESHOLD = 0.35  # Size difference allowed when pairing markers
ANGLE_MAX_DEGREES = 75       # Max angle from horizontal
GATE_MEMORY_FRAMES = 5       # Keep last gate through short detection dropouts
GATE_SMOOTHING_ALPHA = 0.35  # Lower = steadier gate center, higher = more responsive
STEER_ZONE_LEFT = IMG_WIDTH / 3
STEER_ZONE_RIGHT = 2 * IMG_WIDTH / 3

# ===== GPIO PINS =====
LED_PINS = {
    'stop': 17,
    'strong_left': 27,
    'light_left': 22,
    'go': 23,
    'light_right': 24,
    'strong_right': 6
}
SERIAL_TX_PIN = 14
SERIAL_PORT = '/dev/ttyAMA0'
SERIAL_BAUD_RATE = 9600

# ===== GATE CROSSING DETECTION =====
GATE_PRESENT_THRESHOLD = 3     # Frames gate must be visible before it counts (debounce)
GATE_ABSENT_THRESHOLD = 8      # Frames with no gate to trigger clearing phase
COOLDOWN_FRAMES = 15           # Frames to go straight after gate disappears (~0.5s at 30fps)
NO_GATE_CHAR = '?'             # Sent when no gate visible and not in any phase
STRAIGHT_CHAR = 'M'            # Sent during CLEARING phase (center = go straight)
LETTER_NUM_ZONES = 26          # A-Z = 26 zones across frame width

# ===== DEBUG =====
LOG_RUNTIME = True
SHOW_DEBUG = False
STREAM_OVERLAY = False
