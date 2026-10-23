# Vision

Run the hardware-free path from the repository root:

```bash
python3 final_navigation/vision/offline_demo.py --output demo-output
python3 -m unittest discover -s tests -v
```

`requirements.txt` covers desktop OpenCV/NumPy. The full `main.py` runtime additionally needs Picamera2, RPi.GPIO and pyserial, configured Pi hardware, GPIO LEDs and UART. It starts camera capture, serial output and an MJPEG web stream. `main_no_rover.py` and `test_visualise_laptop.py` also access a camera. They are not offline tests.

The detector uses the project's configured custom 4×4 cards. ArUco settings in `config.py` are historical experimental configuration and are not the active detector. `gate_tracker.py` holds short dropouts; `path_finder.py` converts image x into letters and LED steering zones.
