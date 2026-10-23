# Verification — 22 October 2026

Commands run successfully from the release folder:

```sh
python3 -m unittest discover -s tests
python3 analyze.py
```

One regression test covers both all-invalid sentinel cases and a mixed valid/invalid case. It first failed because the reproducible invalid-reading handler was absent, then passed after implementation. `analyze.py` verifies five trials per group and 32 groups, and recomputes `summary.csv`: 160 measurements total. Means and half ranges use valid readings only. Source workbook was read without modification using the bundled Python 3.12.14/openpyxl runtime; exported analysis was checked using Python 3.14 and uses only standard libraries.

The optional MATLAB chart was executed under MATLAB R2024b Update 6, exported to `results.png`, and visually reviewed. A first chart inspection caught material labels being inferred as numbers; forcing the mixed condition column to string fixed the labels. All three panels and their units are readable. The plot uses half ranges, not percentage uncertainty or confidence intervals.

| Master CV claim | Dataset/report evidence | Status |
|---|---|---|
| Five repeated measurements across materials, 0–60° angles, 0–6 cm offsets | 7 material, 5 angle and 4 offset groups for each sensor, five trials each | Reproduced from workbook |
| Ultrasonic unreliable at 45–60° | Means 130.824 and 185.154 cm at nominal 10 cm setup; half ranges 37.885 and 22.445 cm | Supported by recorded readings, not invalid-sentinel counts |
| Ultrasonic sponge failure | All five readings −0.02 cm, documented invalid sentinel | Supported |
| ToF sensing failure with some transparent targets | The report describes an extra angled transparent-acrylic observation; workbook does not tabulate that condition | Qualitative historical evidence only |
| ToF lateral limitation | All five readings 819 cm at 6 cm offset, documented invalid sentinel | Supported |
| Uncalibrated ToF limited absolute-accuracy comparison | Workbook white-paper ToF mean 13.9 cm at nominal 10 cm; no calibration dataset accompanies the experiment | Limitation retained; no calibration claimed |
| Forward ToF and two ultrasonics angled ±25°, rejection/fallback logic | Proposed in report; not tested as a combined system in this data | Design proposal only |

The workbook's original reported uncertainty/error columns were not treated as authoritative formulas. This release recomputes plain means and half ranges from trial cells, excludes 819/−0.02 sentinels, and avoids unsupported calibrated accuracy rankings. The report's prose contains numbers that differ from workbook-derived values; the release uses the workbook trials for quantitative summaries.

The original report, workbook and photographs stay private and are not copied into this package. Sensor identity and sentinel interpretation were confirmed from the report. The acquisition software is credited to a teammate; it is not included or claimed as Emre's code. No sensor was connected, no new measurements were collected, and no publication or permission to publish was established. See `ATTRIBUTION.md`.
