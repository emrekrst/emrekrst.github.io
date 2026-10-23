import csv
import io
from pathlib import Path
import subprocess
import sys

executable = Path(sys.argv[1])
assert executable.is_file(), "build the offline CLI demonstration first"
result = subprocess.run([str(executable)], capture_output=True, text=True)
assert result.returncode == 0, result.stderr
rows = list(csv.DictReader(io.StringIO(result.stdout)))
assert len(rows) == 11
assert rows[0]["accepted"] == "1"
assert rows[1]["accepted"] == "0", "duplicate sequence must not refresh watchdog"
assert 0 < float(rows[0]["left"]) < 1, "first movement must be ramped"
assert float(rows[3]["left"]) > float(rows[0]["left"])
assert rows[7]["watchdog_stopped"] == "1", "100ms without a new command must stop"
assert float(rows[7]["left"]) == float(rows[7]["right"]) == 0
assert rows[9]["accepted"] == "1" and rows[9]["watchdog_stopped"] == "0"
assert 0 < float(rows[9]["left"]) <= 0.02, "recovery starts ramp from zero"
print("offline command/ramp/watchdog demo passed")
