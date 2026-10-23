# Inherited Raspberry Pi baseline (not rerun)

The original `HANDOFF.md` records Raspberry Pi 5 (2 GB), Trixie aarch64 and IMX477 acceptance, with 15/15 CTest targets passing in that earlier environment. It records a 2028×1080, 50 FPS, 12-bit camera profile, a pretracking CAM/VIEW sample of 50.1 FPS, and one locked-target sample of CAM/VIEW 32.3 FPS, TRACK 16.1 FPS, CSRT 62 ms, AGE 69 ms and SKIP 53%, at approximately 221% process CPU and 210 MB RSS.

These are inherited observations, not current verification, guaranteed performance, or a substitute for physical testing. The handoff explicitly leaves moving-target and forced-loss physical acceptance outstanding. Original deployment paths and private screenshots are omitted from this release candidate. Current desktop verification is documented separately.

The master CV also describes Python/Optuna experiments using IoU and latency. Their executable tuning scripts and experiment records were not present in the copied source tree. This package demonstrates configurable CSRT and measured runtime telemetry, and does not claim to reproduce those experiments.
