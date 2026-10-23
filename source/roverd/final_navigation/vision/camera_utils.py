import time

from picamera2 import Picamera2

from config import (CAMERA_BUFFER_COUNT, CAMERA_RAW_HEIGHT, CAMERA_RAW_WIDTH,
                    CAMERA_STARTUP_RETRIES, FRAMERATE, IMG_HEIGHT, IMG_WIDTH)


def open_camera():
    """Create, configure, start, and sanity-check the Pi camera."""
    last_error = None

    for attempt in range(1, CAMERA_STARTUP_RETRIES + 1):
        picam2 = None

        try:
            picam2 = Picamera2()
            cam_config = picam2.create_video_configuration(
                main={"size": (IMG_WIDTH, IMG_HEIGHT), "format": "RGB888"},
                raw={"size": (CAMERA_RAW_WIDTH, CAMERA_RAW_HEIGHT)},
                controls={"FrameRate": FRAMERATE},
                buffer_count=CAMERA_BUFFER_COUNT,
            )
            picam2.configure(cam_config)
            picam2.start()
            picam2.capture_array()
            print(
                f"Camera ready: {IMG_WIDTH}x{IMG_HEIGHT} RGB888, "
                f"raw={CAMERA_RAW_WIDTH}x{CAMERA_RAW_HEIGHT}, "
                f"{FRAMERATE} fps"
            )
            return picam2
        except Exception as exc:
            last_error = exc
            if picam2 is not None:
                try:
                    picam2.stop()
                except Exception:
                    pass
                try:
                    picam2.close()
                except Exception:
                    pass

            print(f"[camera] open attempt {attempt}/{CAMERA_STARTUP_RETRIES} failed: {exc}")
            time.sleep(0.5)

    if last_error and "resource busy" in str(last_error).lower():
        raise RuntimeError(
            "Camera is already in use. Stop any other main.py, main_no_rover.py, "
            "rpicam-hello, libcamera, or Python process that has the camera open, "
            "then rerun this script."
        ) from last_error

    raise RuntimeError(
        "Camera started but no frame arrived. Reseat the ribbon cable at both "
        "ends, confirm the cable contacts face the right way, try a shorter or "
        "known-good cable, and check Pi power before rerunning."
    ) from last_error
