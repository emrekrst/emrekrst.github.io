# Serial-controlled PWM DAC with PID feedback

An ATmega644P coursework example combining 8-bit PWM, ADC feedback, a serial command interface and a Python/Tkinter control panel with a 50-sample freehand waveform editor. The source implements bounded integral accumulation and PWM saturation. An external RC low-pass filter converts PWM into an approximate analogue voltage.

## What was recovered

- `embedded_skeleton.c`: AVR application, including manual/PID operation and ADC monitoring.
- `debug.h`: supplied UART stream library credited to Steve Gunn; its existing Creative Commons Attribution notice is retained.
- `gui_plus.py`: recovered GUI including waveform drawing. `gui.py` now launches this consolidated GUI.

**`pwm_cli.cpp` is a newly reconstructed POSIX C++ client. The historical C++ host CLI mentioned in the master CV was not recovered.** `protocol.c/.h`, tests and build/docs were added during preparation. The recovered firmware's unsafe command parsing and truncated-line handling were fixed; ADC targets and PWM are bounded, and firmware gain commands now match the recovered GUI controls. These are changes to the standalone copy only.

## Build and test without hardware

Requires GCC/Clang, Make, Python 3 with Tk, and pySerial. AVR compilation requires avr-gcc and avr-libc.

```sh
python3 -m venv .venv
. .venv/bin/activate
python3 -m pip install -r requirements.txt
make
make test
make firmware                 # AVR_CC=/path/to/avr-gcc if needed
```

`make test` opens local pseudoterminals, not physical serial devices. It checks command framing, parser bounds/ranges, GUI logic and C++/pySerial transport. `make firmware` only builds an ELF; it does not flash a board. Outputs remain in ignored `build/`.

## Host use with a separately verified circuit

```sh
python3 gui_plus.py
./build/pwm_cli /dev/ttyUSB0 READ
./build/pwm_cli /dev/ttyUSB0 SET 128
./build/pwm_cli /dev/ttyUSB0 AUTO 512
./build/pwm_cli /dev/ttyUSB0 MONITOR    # Ctrl-C sends STOP and exits
```

Select the actual port in the GUI. The client examples are usage instructions; no physical port or board was accessed during preparation. The new CLI supports macOS/Linux POSIX serial devices at 9600 baud, 8 data bits, no parity, one stop bit. It does not provide a Windows backend.

## Firmware connections and assumptions

The source configures a **12 MHz** CPU clock, UART0 at **9600 8N1**, timer0 fast PWM on **PORTB bit 3 / OC0A**, and ADC **channel 0** feedback. The RC-filter output feeds ADC0 and the measurement instrument; all signal grounds must share a reference. Connect UART0 through a logic-level-compatible USB–UART adapter. Package pin numbers depend on the actual MCU package and must be checked against its datasheet.

The ADC selects **AVCC** as reference (`REFS0`). The GUI's voltage conversion assumes that reference is **3.3 V**, as stated in the CV. Confirm actual AVCC, PWM high voltage and ADC input limits before using that conversion. No recovered schematic, RC component values or load value is included; choose and measure a circuit appropriate to the board rather than treating the display labels as calibration.

## Command schema

Commands are ASCII, case-sensitive and terminated by CR or LF. Oversized lines (more than 49 characters) are discarded entirely. Missing numeric arguments, nonnumeric/trailing data, unknown tokens and unsupported extra arguments produce `ERR invalid command`.

| Command | Effect / units |
| --- | --- |
| `SET n` | Disable PID and monitoring; PWM saturated to 0–255 |
| `AUTO n` | Enable PID, reset history; ADC target saturated to 0–1023 |
| `READ` | One `ADC: n | PWM: n` response |
| `MONITOR` | Start repeated ADC/PWM text readings |
| `STOP` | Stop monitoring; **does not disable PID or force PWM to zero** |
| `KP n`, `KI n`, `KD n` | Gain in thousandths, integer range 0–30000 |

Any incoming byte also stops monitoring, matching the original firmware. `SET 0` explicitly disables PID and commands zero duty cycle. PWM waveform playback sends 50 duty-cycle samples using cancellable Tk callbacks scheduled 50 ms apart; it is not a precision waveform generator. Serial delays and OS scheduling affect timing. Mode changes cancel queued waveform callbacks; playback and UI commands execute on the same Tk thread.

The original discrete PID is empirically tuned in iterations; it has no measured fixed-period scheduler or explicit time scaling. The nominal 100 µs loop delay excludes ADC/serial execution time. Monitor timing and waveform amplitude/frequency require bench measurement.

## Scope and rights

This project corresponds to the master CV's serial PWM DAC project. Historical oscilloscope, load-transient and physical voltage-regulation statements come from that CV and were **not reverified** here. No new physical measurement is claimed. See [VERIFICATION.md](VERIFICATION.md).

No project-wide licence has been invented. Keep the supplied UART attribution; its header does not specify a CC version, and other source rights need confirmation before public release.
