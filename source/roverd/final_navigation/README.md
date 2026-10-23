# Navigation source

- `vision/`: custom marker detection, gate pairing/tracking, steering and Pi runtime.
- `firmware/`: Pico C++ UART receiver, motor PWM and failsafe state machine.

Start with the repository [README](../README.md) for the offline demonstration and build commands. This source is camera-based navigation; the CV's later MicroPython ToF fallback is not in this package.
