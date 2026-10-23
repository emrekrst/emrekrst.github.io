"""Recompute summaries from anonymised trials. Python standard library only."""
from pathlib import Path
import csv
from collections import defaultdict
from statistics import mean

def summarize(sensor, readings):
    sentinel = {'ToF': 819.0, 'Ultrasonic': -0.02}[sensor]
    valid = [x for x in readings if x != sentinel]
    return (len(valid), len(readings)-len(valid), mean(valid) if valid else None,
            (max(valid)-min(valid))/2 if valid else None)

def main():
    root = Path(__file__).resolve().parent
    groups = defaultdict(list)
    with (root/'data/trials.csv').open(newline='') as f:
        for row in csv.DictReader(f):
            groups[(row['sensor'],row['experiment'],row['condition'])].append(float(row['distance_cm']))
    with (root/'data/summary.csv').open('w',newline='') as f:
        writer=csv.writer(f)
        writer.writerow(['sensor','experiment','condition','valid_count','invalid_count','mean_cm','half_range_cm'])
        for key,readings in groups.items():
            assert len(readings)==5, f'Incomplete trials for {key}'
            writer.writerow([*key,*summarize(key[0],readings)])
    assert len(groups)==32
    print('PASS: 160 trials, 32 conditions; invalid sentinels excluded from means')
if __name__=='__main__': main()
