# Turbojet speed-control simulation

An educational MATLAB/Simulink model by Emre Karausta, developed independently during an internship learning exercise. Two illustrative plants share a throttle/altitude speed schedule, steady-state fuel feedforward, a PI correction, acceleration/deceleration fuel schedules, a fuel rate limiter and transport delay. One plant uses a first-order speed response; the other uses a nonlinear normalized-spool equation.

The lookup tables and spool coefficients are illustrative generated inputs. They are not validated engine maps. Fuel units were ambiguous in the original comments, so numerical fuel values are reported in model units. RPM is a percentage of nominal speed.

## Run

Requires MATLAB R2024b and Simulink.

```sh
python3 verify.py
```

`MATLAB_BIN` can select MATLAB. Close already-open target models before running; preflight refuses them without changing or closing them. Unrelated models are left intact. The wrapper isolates preferences, working directory and temporary files, enforces a 600-second process timeout, and writes generated traces/cache to ignored `results/`. `run_all.m` initializes every required variable through `Simulink.SimulationInput`; no manual `datas` run or base-workspace setup is needed.

For each plant, the runner tests the saved throttle step, constant zero throttle, and a full-throttle-to-idle command. It checks finite nonnegative RPM, fuel target bounds, PI clamping configuration and final tracking within one RPM percentage point. Logging and scenario overrides exist only in memory. No original model tuning was changed.

Anti-windup is the PI block's own output clamping. External acceleration, deceleration and rate limits do not feed an explicit tracking anti-windup loop. The tests demonstrate simulated behavior under the documented scenarios, not engine-operating limits or production FADEC performance.

See [VERIFICATION.md](VERIFICATION.md), compact `evidence/` and [ATTRIBUTION.md](ATTRIBUTION.md). No hardware was connected or actuated, and nothing was published.
