# Verification — 22 October 2026

## Fresh results

Tested on macOS 15.7.3 arm64, AppleClang 17, CMake 4.3.0, Qt 6.11.1, GStreamer 1.28.5 and OpenCV 4.14.0. No camera or remote host was contacted.

Commands below were run from the preparation workspace (the directory containing `outputs/` and `work/`):

```bash
cmake -S outputs/github-ready/csrt-tracker -B work/vision-rover/csrt-build -DCMAKE_BUILD_TYPE=Release
cmake --build work/vision-rover/csrt-build -j6
QT_QPA_PLATFORM=offscreen ctest --test-dir work/vision-rover/csrt-build --output-on-failure
QT_QPA_PLATFORM=offscreen CSRT_DEMO_SCREENSHOT="$PWD/outputs/github-ready/csrt-tracker/samples/tracking-demo.png"   work/vision-rover/csrt-build/main_window_tests recordedDemoTracksMovingTarget
```

Result: build passed; **15/15 CTest targets passed, zero failed**: script_workflow, camera_config, runtime_telemetry, tracking_frame, seeded_object_selector, csrt_tracker, tracking_controller, video_geometry, placeholder_tracker, frame_mailbox, camera_pipeline, file_pipeline, target_overlay, video_canvas and main_window.

The new `recordedDemoTracksMovingTarget` test opens the bundled six-second MJPEG/AVI recording through the actual FilePipeline and MainWindow, seeks to the first frame, invokes the shared target-selection method, waits for at least 15 asynchronous CSRT updates, and requires IoU > 0.4 against known moving-target coordinates. It passed and generated the included screenshot with a locked target and telemetry. This is generated prerecorded input, not footage from a physical camera. Existing tests also cover selection failure, occlusion/loss, seeking, frame dropping and geometry.

The test screenshot was visually inspected. Offscreen Qt emits benign font/plugin warnings and macOS GStreamer can emit an NSApplication/GL warning; no assertions or tests failed. Screenshot telemetry is one synthetic 10 FPS desktop run, not a Pi performance claim.

## Changes and regression evidence

- Added a generated recording, generated selector fixtures, generator script and recorded-video integration check. The new recording test failed before the sample existed and passed after generation.
- Replaced two inherited photographic road/car crops with generated PNG fixtures because the original media provenance was unknown. Their original regression checks passed during the initial local baseline; final public tests use clearly named generated-fixture cases. Real-photo generalization is therefore not established by the distributed fixtures.
- Removed embedded personal SSH/home paths. Sync requires `RPI_TARGET` and accepts `RPI_REMOTE_ROOT`; tests use fake SSH/rsync executables and verify abort-on-SSH-error and absence of deletion flags.
- Isolated the run-script default-environment test from the enclosing offscreen CTest environment. The failure was reproduced with `QT_QPA_PLATFORM=offscreen` before the test fix; production GUI/CSRT algorithms were unchanged.
- Excluded generated builds, private footage, agent plans and stale tuning instructions. Original source files were not edited.

## Master-CV claim boundaries

| Claim | Evidence in this copy |
|---|---|
| Qt/OpenCV tracking application, files and single-click selection | PASS: desktop build, recorded-video integration and unit tests |
| Latest-frame mailbox and asynchronous CSRT | PASS: frame/controller tests and real recording through the controller |
| Configurable parameters and runtime telemetry | PASS: config/telemetry tests, screenshot |
| Raspberry Pi 5 + IMX477 live operation | NOT_RUN: no Pi or sensor used; inherited baseline in `docs/HISTORICAL_EVIDENCE.md` |
| Physical preview/latency/FPS values | NOT_RUN: historical observations only, no fresh hardware benchmark |
| Python/Optuna tuning experiments | NOT_REPRODUCED: executable tuning scripts/results were not recovered |

Logs are retained outside the release folder under `work/vision-rover/` (`csrt-build-final.log`, `csrt-ctest-final.log`, `csrt-demo-after.log`). Nothing was installed, deployed, committed or published.
