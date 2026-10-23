# Verification — 22 October 2026

## Fresh results

macOS arm64; Python 3.14.6, NumPy 2.5.1, OpenCV 4.14.0, CMake 4.3.0, AppleClang 17. Native behavioral tests use the production firmware header with fake UART/clock/output boundaries. Pico firmware was separately cross-compiled using installed Pico SDK 2.2.0 and Arm GNU Toolchain 14.2.Rel1 (GCC 14.2.1).

Commands from the preparation workspace:

```bash
PYTHONDONTWRITEBYTECODE=1 PYTHON=/opt/homebrew/bin/python3   outputs/github-ready/roverd/tests/run-offline.sh "$PWD/work/vision-rover/rover-host"
PYTHONDONTWRITEBYTECODE=1 /opt/homebrew/bin/python3   outputs/github-ready/roverd/final_navigation/vision/offline_demo.py   --output outputs/github-ready/roverd/samples
cmake -S outputs/github-ready/roverd/final_navigation/firmware -B work/vision-rover/pico-build   -DPICO_SDK_PATH="$HOME/.pico-sdk/sdk/2.2.0"   -DPICO_TOOLCHAIN_PATH="$HOME/.pico-sdk/toolchain/14_2_Rel1"   -DPICOTOOL_FETCH_FROM_GIT_PATH="$PWD/work/vision-rover/picotool"
cmake --build work/vision-rover/pico-build -j4
```

**PASS:** six Python tests (end-to-end demo, blank scene, rotation, gate rejection, dropout expiry/recovery, steering limits/case) and four native firmware scenarios (`protocol`, `overflow`, `zero`, `center`). The demo detected exactly two markers, center `[639.5, 339.5]`, command `go`, UART letter `M`, hardware_used `false`. Generated image inspected visually.

**PASS:** real RP2040 firmware compile/link and UF2 generation. Generated `rover_move.uf2` is 52,736 bytes; SHA-256 `6a62bddfe2894c8d1410f02da5b46b89317555dd45206206894aacc5fdabcf99`. Build artifacts remain in `work/`, not the release copy. Picotool was fetched/built into `work/`; no SDK files or installed tools were modified. Nothing was flashed.

## Reproduced issues fixed in the copy

1. A line containing 64 junk bytes followed by `A\n` was wrongly accepted as `A`. A failing host test reproduced it; the parser now discards through the delimiter and then accepts a fresh valid line.
2. A valid command received at time zero never timed out because zero was also the never-received sentinel. A failing test reproduced it; receipt is now tracked separately. Tests cover the 300/301 ms boundary and unsigned clock rollover.
3. The configured K-P straight band produced unequal wheel demand because zone 12 still yielded a negative steering fraction. A failing test reproduced it; the deadband now yields zero steering and equal wheel output.
4. CMake documented `-DPICO_SDK_PATH` but read only the environment variable, attempting `/external/pico_sdk_import.cmake`. The failed configure was captured, then the copy was fixed to support both forms with a clear missing-SDK error. The real build passed after the fix.

Production vision algorithms are unchanged. Added a hardware-free demo, tests and explicit desktop NumPy/OpenCV dependencies; Pi-only dependencies are documented separately.

## Master-CV claim boundaries

| Claim | Status |
|---|---|
| Custom-marker camera gate/steering pipeline | PASS offline generated scene; physical performance NOT_RUN |
| C++ Pico UART parsing, software timeout and wheel slew | PASS native behavior tests and actual cross-build; physical motor output NOT_RUN |
| Deployed MicroPython single-forward-ToF fallback, stopping/settling/alternating turns | NOT_FOUND in recovered source; NOT_REPRODUCED by this C++ camera stack |
| Empirically calibrated 90° turns, power distribution, centre of gravity, indoor trials/race result | NOT_RUN; documentary/historical claims outside these software tests |

No camera, GPIO device, serial port, motor, Pi or remote host was opened. `main.py` remains a hardware runtime and should only be run on the configured rover. Original project files were untouched. Existing group work is credited; no blanket license was inferred and nothing was published.

Logs outside the release folder: `work/vision-rover/roverd-tests-final.log`, `firmware-*-before.log`, `pico-configure-before.log`, `pico-configure.log`, and `pico-build-final.log`.
