# Verification — 22 October 2026

## Commands and environment

From the release folder: `python3 verify.py` (exit 0). It starts `/Applications/MATLAB_R2024b.app/bin/matlab -batch` with `test_release; test_torque_limits; run_all(true, artifact_dir)`, using absolute addpaths, temporary cwd/preferences/TMPDIR, and a 600-second process timeout. All execution used copies. Compact actual output and numerical JSON are in `evidence/`.

MATLAB 24.2.0.2923080 (R2024b) Update 6 on macOS 15.7.3, Apple Silicon. Available and exercised: Simulink, Control System Toolbox, Signal Processing Toolbox, Simscape, Simscape Multibody, all version 24.2. Symbolic Math Toolbox 24.2 was installed; the existing generated ODE was used rather than regenerating it. No toolbox/license blocker occurred.

`test_release` passes numerical resonance, body-mode, Rayleigh, point-mass ratio, phase-margin, unstable-pole and nonlinear passivity checks. `test_torque_limits` passes both ±0.5 rad commands. Six physical/signal models execute: two-cart 10 s, each of three beams 10 s, nonlinear cart–pendulums 100 s, helicopter 10 s. Expected MATLAB warnings report instability when evaluating the motor-lag loop. Startup emits Java X11-package warnings; they did not prevent execution.

## CV claim comparison

| Claim | Current evidence | Status |
|---|---|---|
| Cart resonances ≈17.5 and 49.4 rad/s | Peaks of cart-1/force-1 frequency response: 17.516005 and 49.429514 rad/s. Undamped modes: 17.521402 and 49.426718 rad/s. | Reproduced |
| Cart free/forced MATLAB and equivalent Simscape responses | Analytical model computed; saved forced Simscape model ran 10 s and outputs were captured. This release does not assert a quantitative waveform-alignment tolerance. | Executed; historical “near-identical” wording remains qualitative |
| Body soft soil 14.20, 1.73 Hz; hard soil 32.30, 1.76 Hz | Distinct positive imaginary parts of full damped eigenvalues: 14.19986747, 1.73259755; 32.29505230, 1.76015436 Hz. | Reproduced |
| All five undamped body modes calculated | Soft: 1.676000, 7.075063, 10.703647, 19.182925, 209.117485 Hz. Hard: 1.724382, 7.176866, 10.703876, 28.657886, 255.397995 Hz. Original `bs=2170e3` remains unchanged. | Reproduced |
| Simply supported beam first three modes and Rayleigh estimate | 6.535654705, 26.142618820, 58.820892344 Hz. Rayleigh relative error 2.72e−16 using the exact first-mode trial shape. | Reproduced |
| Midspan mass 1.5× beam mass for half frequency | Rayleigh modal-mass calculation gives 351 kg for a 234 kg beam. This is an assumed-shape estimate; a point mass changes the exact mode shape. | Reproduced within stated approximation |
| Beam Simscape errors 0.15% cantilever and 0.37% simply supported | Measured 2.329244098 Hz versus analytical 2.328306262 Hz: 0.040280%. Measured 6.559697358 Hz versus 6.535654705 Hz: 0.367869%. | Simply supported matches rounding; cantilever 0.15% not reproduced |
| Tuned beam error 0.62% against half original analytical frequency | Measured 3.256240061 Hz versus target 3.267827352 Hz: 0.354587%. | 0.62% not reproduced |
| Nonlinear MATLAB/Simscape overlap over 100 s | Both run for 100 s. After original cart-coordinate offsets −0.55/+0.55 m, maximum differences are 0.00092576 m, 0.00147944 m, 0.01798834 rad and 0.01438680 rad. ODE energy decreases throughout sampled trajectory. | Duration reproduced; exact agreement not established |
| Helicopter torque limits −6/+10 Nm | Original saved model had no limiter: +0.5 rad commanded 20 Nm. Added explicit saturation to release copy. ±0.5 rad regressions now stay within limits. | Missing enforcement fixed in copy; not evidence that historical original enforced limits |
| Helicopter 0.119 s settling, 0.88% overshoot for 0.25 rad | Saved model/release gains Kp=40, Kd=2.42979296119, Ki=0, J=0.073824; derivative acts on pitch feedback. Released Simulink result: 0.25525646 s, 4.26352861%. Independent transfer calculation: 0.25617596 s, 4.32569854%. Settling is MATLAB `stepinfo` default 2%; step at t=0, no motor lag. The .25 rad response does not exceed the added limits. | CV response metrics not reproduced; gains were not tuned to match them |
| Ideal phase margin 65.5° at 36.2 rad/s | 65.52388666° at 36.16318336 rad/s. | Reproduced |
| 0.1 s motor lag gives −13° at 19.6 rad/s and instability | −13.00413120° at 19.57019933 rad/s; closed loop unstable. | Reproduced |
| Infinite gain margin for both loops | R2024b `margin` returns 0 at zero-frequency phase crossover for both integrator loops. `allmargin` returns `[0 Inf]` for the motor-lag loop, with frequencies `[0 Inf]`. | Blanket “infinite” statement not reproduced under this MATLAB convention |
| Inverted-pendulum poles ≈±7 rad/s | −7.003570518, +7.003570518 rad/s. Original lead and servo analysis script included. | Pole calculation reproduced |
| Bytronic rig balancing, measured gain limits, movable-mass effects | Historical CV/lab statements only. No physical rig or experimental traces were included in this run. | Not retested |

Beam frequency extraction uses positive displacement peaks after 5.1 s (free response after the force pulse), three-sample quadratic interpolation in actual simulation time, then reciprocal median peak spacing. Errors above are calculated from measured signals, not copied from the CV. Different cursor-based or spectral estimators may give different rounded errors; their historical settings were not retained.

The nonlinear ODE uses g=9.81 while the saved Simscape model uses 9.80665 m/s². A separate diagnostic using the saved model's gravity reduced angular differences to approximately 0.0033/0.0048 rad, but residual differences remained and that diagnostic did not meet a 0.001-unit comparison test. It was not used to overwrite the baseline results or claim exact matching.

## Defects and red/green evidence

Before the harness existed, the release lacked clean-start execution and original plotting scripts required manually generated `out` data. Initial checks failed; the release now runs without that base-workspace dependency. A first integration run also exposed a real logging mismatch: original `tout` was capped at 10,000 samples while nonlinear output arrays held 50,002 samples. The helper now captures individual full timeseries. Another initial logger attempted duplicate named branches; deduplicated logging now supports the turbojet package independently.

The helicopter behavioral regression failed with 20 Nm for a 0.5 rad command. Adding the single saturation block produced passing +0.5 and −0.5 rad tests. Original gains and dynamics were retained. The original model is unchanged outside this release; the baseline failure log is retained in compact form.

## Suggested precise CV wording

- Beam: “Validated simulated beam frequencies against analytical references; current reproducible extraction gives approximately 0.04% cantilever, 0.37% simply supported, and 0.35% against the half-frequency tuning target.” Retain historical numbers only if their original measurement method can be produced.
- Nonlinear: “Cross-compared MATLAB and Simscape responses over 100 seconds, aligned coordinate references and quantified peak differences of about 1.5 mm and 0.018 rad in the retained models.”
- Helicopter: “Implemented derivative-on-feedback PD control; the retained ideal model gives about 0.256 s settling time and 4.3% overshoot for a 0.25 rad command. Reproduced the loss of stability with 0.1 s motor lag.” Describe torque saturation as an added release correction unless historical evidence supports earlier enforcement. Report phase margins without an unqualified infinite gain-margin claim.

No physical validation, employer/university publication permission, or public licence was established by these tests. No originals were edited and no remote, commit, push or publication was performed.

A final independent review found that broad `bdclose('all')` cleanup could close unrelated unsaved models. A behavioral sentinel regression reproduced that failure. The broad cleanup was removed. The simulation helper now refuses already-loaded target models before mutation, with an actionable `release:ModelAlreadyLoaded` error. Final targeted MATLAB command `test_model_ownership; test_release; test_torque_limits` exited 0 after that lifecycle-only correction; it checks unrelated unsaved models on success/error and preserves a preloaded same-name target. The full six-model numerical run above preceded this lifecycle correction. See the retained red/green ownership logs.
