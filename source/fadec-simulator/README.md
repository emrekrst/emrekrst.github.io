# Triple Redundant FADEC Simulator and SCADA

A browser-based educational engine-control simulation co-developed by Demir Akın and Emre Karausta using AI-assisted code generation and integration. It demonstrates a simplified plant, PI fuel control, three-channel voting, fault responses and SCADA-style instrumentation. The existing MIT licence and in-app contributor credits are retained.

## Run

Open `index.html` in a modern desktop browser. No installation, server, account or build is required. Select **EN** or **TR** for the interface language. The application uses a fixed 50 ms simulation step (20 Hz); browser scheduling can run slower under load.

## Demonstration

1. Click **START ENGINE** and watch CRANK, LIGHT-OFF, ACCEL and RUN. Move the thrust lever to observe the spool and temperature response.
2. Select **Half-blind**: channel C's exhaust-temperature fault reduces availability to two channels. **Leader handover** isolates A and transfers authority to B.
3. Select **Common-mode**: all three N2 channels agree on the biased reading, while the onboard model detects the inconsistency. Contrast with **Total loss** (model synthesis) and **Open-loop** (measurement loss together with model failure).
4. Try **Force: Overspeed** to latch protection and reduce commanded fuel. Clear faults to recover. Arm **Start: Hot** or **Start: Hung** before starting to demonstrate aborted starts.
5. Enable **IETM**, select an indicator and expand its live explanation. **AUTO · FAB** exposes the simulated mission and decision log.

Reload the page to repeat from the identical initial state and seeded noise sequence. Clear-faults resets injected/latching faults, not the entire plant or random-number sequence.

## Tests

Node.js 20 or newer is needed only for tests:

```sh
npm install
npx playwright install chromium
npm test
```

On macOS the suite also discovers an installed Google Chrome. Alternatively set `PLAYWRIGHT_CHROMIUM_EXECUTABLE` to a Chromium/Chrome executable. Set `EVIDENCE_DIR=evidence` to regenerate screenshots and scenario measurements.

The tests drive the real HTML application in Chromium, including the model and UI controls. Most advance deterministic steps directly, so a simulated startup does not require a real-time wait. See [verification](VERIFICATION.md) and [scenario measurements](evidence/scenarios.json).

## Scope

This is a synthetic educational simulation, not production engine software, hardware-in-the-loop validation, aircraft certification evidence or a model calibrated against a particular engine. Fault scenarios demonstrate this implementation's behavior under stated assumptions. The CV's co-developed browser-simulation claim is supported; real ECU communication and thermocouple firmware exercises are separate work and are not supplied here.

The release copy corrects displayed 50 Hz labels to the application's actual 20 Hz update rate; it does not change plant/control parameters. A static footer that incorrectly asserted normal health during faults is relabelled as an educational simulation. Before publication, confirm that both contributors approve distributing the co-developed material and that internship-related material is cleared.
