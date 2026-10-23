# Rover command, ramp and watchdog demo

A small hardware-free extraction from an internship rover control project. It preserves the original C++ command parser/sequence gate, keyboard mixer, acceleration/deceleration ramp, and normalized differential-drive controller with a command watchdog. It does not contain ROS, serial drivers, motor control transports or deployment configuration.

## Run

Requires a C++17 compiler, CMake 3.20+ and Python 3 for the CLI test.

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
./build/rover_demo
```

The CLI uses deterministic simulated time and prints CSV. It accepts a `ROVER1` command at 0 ms, rejects its duplicate at 20 ms, accepts the next sequence at 40 ms, rejects a malformed packet at 60 ms, stops at 140 ms when command age reaches 100 ms, and resumes from zero after a fresh command at 180 ms. See [samples/demo.csv](samples/demo.csv).

`include/drive_logic.hpp`, `include/motor_teleop_v2/motor_control_core.hpp`, `include/remote_mvp/remote_command.hpp` and their original tests were extracted without modification. `src/demo.cpp` and its subprocess test are a new demonstration harness. The sequence gate deduplicates within the current session; it is not an authentication or general replay-protection mechanism. This CLI never binds a socket or opens a serial port.

## Evidence and rights

See [VERIFICATION.md](VERIFICATION.md) for results and limits. This sample supports the command/ramp/watchdog aspect of the project. It does not reproduce Jetson deployment, F310 input, RealSense video, ROS 2 telemetry, BusLinker/servo communication, or the historical UART baud diagnosis.

Prepared from Emre Karausta's internship working copy, retaining original source comments. No source license was present in this extraction and no blanket license is inferred. This is a local release candidate; nothing has been published.
