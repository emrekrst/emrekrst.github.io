# Verification record

Verified 22 October 2026 on macOS arm64 using Node.js, Playwright 1.62.1 and installed Google Chrome in an isolated headless session. No real engine, aircraft, motor or controller was connected.

## Result

**19/19 browser tests passed.** Reproduce with `npm test` after the test dependencies in README are installed. Evidence regeneration: `EVIDENCE_DIR=evidence npm test`.

- Startup reaches RUN and approximately 60% N2 at idle; a 75% lever command raises N2, and shutdown spins down. The actual start button and thrust slider are used.
- All ten scenario buttons exercise their real handlers: nominal, half-blind, dual isolation, leader handover, total loss, open-loop, single-channel, common-mode, onboard-model crash and CCDL loss. Assertions check the expected availability, isolation, leader, synthesis, model residual or link state, plus fault-log latching and recovery.
- Hot and hung starts abort with zero commanded fuel; clearing faults permits a new start.
- Overspeed latches and commanded fuel is reduced to no more than 120 model units until reset. The forced-overspeed injection intentionally keeps plant speed high; this test proves simulated protection output, not real deceleration.
- Every 16-bit fault word round trips through the real encoder/decoder (65,536 values).
- Repeating an identical input sequence on a fresh page produces an identical model state.
- English/Turkish controls and expanded technical-help close behavior work.
- The ordinary browser timer advances the model through the briefing start button.
- No uncaught page errors during the tests.

## Reproduced defects and minimal corrections

The first browser suite passed 16 of 18 checks. The two failures established that the displayed frequency (50 Hz) contradicted the running timer (50 ms / 20 Hz). The labels and live-help explanation were corrected in the copy, preserving the plant and control parameters. A separate failing regression established that the static footer said SYSTEM NOMINAL during a channel-isolation fault; it now identifies the application as an educational simulation. The final expanded suite passed all 19 tests.

## Master CV correspondence

The supplied application supports the co-developed, AI-assisted browser FADEC/SCADA simulation claim: simulated engine response, redundancy states and fault responses are operational in the copied application. The retained credits distinguish the contributors. The separate embedded ECU-header and MAX31855 exercises in the CV are not included and are not claimed to be verified by these browser tests.

## Limits

The tests cover this synthetic model and interface. They do not calibrate it to an engine, validate aerothermal fidelity, certify fault coverage, prove bumpless transfer at every operating point or substantiate real hardware safety. Screenshots were visually inspected. Contributors and internship owners must clear the co-developed content before publication; the existing MIT file has been preserved without inventing broader permissions.
