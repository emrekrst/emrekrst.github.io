# Verification — 22 October 2026

macOS 15.7.3 arm64, AppleClang 17, CMake 4.3.0, Python 3.14.6. No hardware or network activity.

Commands from the preparation workspace:

```bash
cmake -S outputs/github-ready/rover-control-demo -B work/vision-rover/control-build -DCMAKE_BUILD_TYPE=Release
cmake --build work/vision-rover/control-build -j4
PYTHONDONTWRITEBYTECODE=1 ctest --test-dir work/vision-rover/control-build --output-on-failure
work/vision-rover/control-build/rover_demo > outputs/github-ready/rover-control-demo/samples/demo.csv
```

**PASS: 4/4 CTest targets**, zero failures: `drive_logic`, `motor_control_core`, `remote_command`, and `demo`. The first three are the extracted original tests and cover keyboard mixing, ramps/reversal, emergency stopping, watchdog boundaries, invalid/nonfinite input, normalized motor commands, protocol parsing, bounds, and duplicate sequence rejection.

The new CLI subprocess test failed before the CLI existed and passed after the harness was implemented. It checks actual CSV output: acceleration starts at 0.015, a duplicate is rejected, fresh commands ramp, watchdog expiry at command age 100 ms zeros both wheels, and a fresh packet resumes from zero. The included deterministic sample has 11 rows (0–200 ms, 20 ms steps). The three extracted production headers and their tests match the source working copy byte-for-byte.

This demonstrates the command/ramp/watchdog subset of the master CV's rover control platform. Jetson/Ubuntu/ROS 2 deployment, USB/UART BusLinker servo integration, RealSense streaming, telemetry, F310 control and UART clock/baud diagnosis are **NOT_RUN**. No claim of physical stopping distance or reliable network operation follows from this isolated software demo. No motors, cameras or serial ports were activated. No full rover stack or deployment paths were included.

Logs retained outside the release folder: `work/vision-rover/control-build.log`, `control-ctest-final.log`, `control-demo-before.log`. No original files were modified and nothing was committed or published.
