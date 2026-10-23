# Verification

Executed on 2026-10-22 on macOS arm64: Apple clang 17.0.0, Python 3.14.0, pySerial 3.5, Homebrew avr-gcc 9.5.0. No board, physical serial port, camera or motor was opened or operated.

```sh
make test
make firmware AVR_CC=/opt/homebrew/bin/avr-gcc
```

Both commands PASS, exit 0, no compiler warnings. Firmware uses `-mmcu=atmega644p -DF_CPU=12000000UL -Os` and links the shared parser with the application and supplied UART header.

- Shared C parser/framing tests execute with AddressSanitizer and UndefinedBehaviorSanitizer: missing/long/unknown tokens, malformed and overflowing integers, extra parameters, SET/AUTO saturation, gains, READ/MONITOR/STOP, oversized-line rejection and recovery.
- GUI method tests check command formatting, acknowledgement-before-ADC handling, ADC-to-voltage display, and bounds of the actual 50-sample drawing array. Additional playback regressions verify cancellation on STOP/AUTO/manual/monitor/disconnect, a single loop after repeated starts, and all 50 samples wrapping in order.
- Reconstructed C++ CLI exchanges SET/AUTO/READ/KP/STOP commands through local pseudoterminals, receives expected text responses and rejects a missing argument before opening a device.
- Recovered GUI's actual `read_once`/`send_cmd` methods exchange a READ/ADC transaction using real pySerial over a pseudoterminal; widget stand-ins allow headless testing.
- Python files compile via `py_compile`.

Before the parser fix, an extracted copy of the original `process_command` produced an AddressSanitizer **stack-buffer-overflow** for a 47-character token. The original GUI regression failed because it sent `READ 0`; after that fix it also needed to skip older acknowledgement lines to display the requested ADC reading. The CLI transport tests were introduced before the new reconstructed client existed.

Fresh review also reproduced a waveform-worker race: a worker paused after its loop condition could send a stale `SET 128` after `AUTO 512`, disabling PID in the firmware. The playback regression failed against the thread-based implementation. Playback now schedules a single cancellable Tk callback, so mode changes and waveform writes run serially on the UI thread. Stop/mode changes/disconnect cancel the pending callback; repeated starts cannot create duplicate loops. The 50-sample sequence and nominal 50 ms scheduling interval are retained.

The transport peer returns canned protocol responses. It is not an AVR emulator or an electrical/circuit model. AVR compilation verifies syntax/linkage for the target, not real-time or physical behaviour.

## NOT_RUN — hardware and runtime limits

- Flashing, actual UART electrical compatibility/baud timing, board clock and pin wiring.
- PWM voltage/ripple and RC filter characteristics; AVCC/reference calibration and physical ADC accuracy.
- PID stability, step response, integral behaviour under a real load, oscilloscope/load-transient CV measurements.
- Monitor cadence, waveform timing/amplitude/frequency and full GUI interaction with hardware.
- Cross-platform GUI thread/connection stress, Windows support and serial disconnect fault injection.

GUI one-shot reads can block up to the serial timeout, and background monitoring relies on a host thread and waveform callbacks use the Tk event loop rather than deterministic scheduling. These limits must be evaluated on the intended board before bench use. All hardware claims remain historical claims, not results generated during this preparation.
