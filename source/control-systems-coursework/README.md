# Control systems coursework

MATLAB and Simulink studies by Emre Karausta, prepared during Electrical and Electronic Engineering coursework at the University of Southampton. Six folders cover six coursework projects: two-cart dynamics, five-DOF body vibration, beam modes and mass tuning, nonlinear cart–pendulums, helicopter pitch control, and inverted-pendulum lead compensation. Beam analysis and tuning are kept together.

The reproducible entry point is `run_all.m`. `src/` retains the coursework equations and models; several original plotting scripts expect a manually created `out` variable and are included for provenance, not as the clean-start entry point. The runner calculates results directly, runs the six saved simulation models, and logs outputs without saving logging changes into them.

## Run

Requires MATLAB R2024b, Simulink, Control System Toolbox, Signal Processing Toolbox, Simscape and Simscape Multibody. Symbolic Math Toolbox is needed only to regenerate `pendulum_ode.m` using the original `Task_B.m` derivation.

```sh
python3 verify.py
```

`MATLAB_BIN` can specify another MATLAB executable. Close any already-open target model before running; the helper refuses preloaded targets to protect unsaved work. Unrelated open models are left intact. The wrapper uses a temporary working directory and preferences, a 600-second process timeout, and writes generated traces/cache to ignored `results/`. It runs the analytical/nonlinear regression tests and the positive/negative torque-bound tests, then all saved simulation models. Alternatively, add this folder to MATLAB's path and call `run_all(false)` for the analytical and nonlinear ODE results, or `run_all(true, artifact_dir)` for the full run.

## Release changes

- Added a clean-start runner, explicit output logging, full timeseries instead of mixing capped `tout` with longer workspace arrays, and numerical regression checks.
- Added the missing −6/+10 Nm saturation to the copied helicopter model. The original saved model had no torque limiter and commanded 20 Nm for a 0.5 rad step. Both ±0.5 rad commands now stay within bounds. Gains and plant parameters are unchanged.
- Retained all original mechanical constants, including the five-DOF model's unusually large shoe damping coefficient. Matrix linear solves in the runner avoid explicit inverses.
- Retained differences between historical results and current reproduction. The helicopter response, two beam error percentages, and exact nonlinear matching are not presented as reproduced CV numbers.

Read [VERIFICATION.md](VERIFICATION.md) for numerical results, claim status and suggested corrected descriptions. `evidence/` contains compact recorded results. No physical rig was operated. See [ATTRIBUTION.md](ATTRIBUTION.md) before publishing or relicensing these academic materials.
