# Verification — 22 October 2026

`python3 verify.py` completed with exit 0 using MATLAB 24.2.0.2923080 (R2024b) Update 6 and Simulink 24.2 on macOS 15.7.3, Apple Silicon. MATLAB ran in a temporary cwd with isolated preferences/TMPDIR and a 600-second process timeout. Only copies were opened. See `evidence/results.json` and `evidence/verification.log`.

The final wrapper also passes a regression confirming already-loaded targets are refused intact and unrelated unsaved models remain open. All six 90-second scenarios passed: the saved throttle step, constant zero throttle, and full throttle until 45 s followed by zero throttle, for each saved plant. Tests assert finite nonnegative RPM, the 180-unit deceleration fuel floor, acceleration-fuel schedule bounds, PI internal clamping configuration, and final absolute tracking error below one RPM percentage point. The existing rate-limiter blocks remain ±3000 model fuel units/s; a separate numerical slew-rate assertion was not added.

| Plant/scenario | Final RPM % | Target RPM % | Absolute error |
|---|---:|---:|---:|
| First-order, saved 25→30% throttle step | 75.0485098674 | 75 | 0.0485098674 |
| First-order, zero throttle | 40 | 40 | 0 |
| First-order, 100→0% throttle | 40.0000210089 | 40 | 0.0000210089 |
| Nonlinear, saved 50→55% throttle step | 87.5000000000 | 87.5 | 1.9e−11 |
| Nonlinear, zero throttle | 40 | 40 | 0 |
| Nonlinear, 100→0% throttle | 40.0000000066 | 40 | 6.6e−9 |

Maximum fuel-schedule excess was 4.55e−13 (round-off). The full-to-idle target fuel ranged from 180 to 1770.1577 for the first-order plant and 180 to 2629.2714 for the nonlinear plant. Tables and full numerical output are recorded in JSON.

The master CV's simulated throttle regulation and idle-at-zero-throttle claims are supported by these runs. The models contain PI controllers with `AntiWindupMode=clamping` and output limiting enabled. This is local PI clamping; no explicit tracking feedback from the external fuel/rate limiters is present. The report therefore does not claim robust anti-windup under every external saturation combination.

The release fixes clean-start initialization and logging only: all lookup/spool variables are passed explicitly through `Simulink.SimulationInput`, and scenario overrides/logging are in memory. The original `.slx` models and `datas.m` coefficients are unchanged. Red check before the new harness failed because a clean-start runner was absent; final runs need no manual base-workspace setup. During harness development, duplicate named signal branches and an incorrect multi-parameter `setBlockParameter` call were exposed by actual MATLAB runs and corrected before the passing full run.

Initial engine speed is inherited from the saved models, not a physical crank/start sequence. No altitude sweep, validated fuel units, calibrated engine data, real-engine test, physical safety qualification or full operating-envelope proof was performed. Suggested CV wording can remain “illustrative Simulink turbojet speed-control model with PI clamping, fuel schedules and rate limiting; demonstrated simulated RPM tracking and idle retention at zero throttle.” Rights to publish internship-related files remain undetermined; see `ATTRIBUTION.md`. No publishing, remote or hardware action occurred.
