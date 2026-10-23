# gate_finder.py - Find valid gates from detected and identified markers

from config import AREA_MATCH_THRESHOLD, ANGLE_MAX_DEGREES
from math_utils import angle_between, midpoint, distance


def gate_metrics(left, right):
    larger = max(left['area'], right['area'])
    smaller = min(left['area'], right['area'])
    area_diff = (larger - smaller) / larger if larger > 0 else 1.0
    angle = angle_between(left['center'], right['center'])
    width = distance(left['center'], right['center'])
    return area_diff, angle, width


def gate_score(area_diff, angle, width):
    area_quality = max(0.0, 1.0 - (area_diff / AREA_MATCH_THRESHOLD))
    angle_quality = max(0.0, 1.0 - (angle / ANGLE_MAX_DEGREES))
    return width * (0.55 + 0.30 * area_quality + 0.15 * angle_quality)


def find_gates(markers):
    """
    Given a list of identified markers, find all valid gate pairs.

    Each marker dict (from square_finder + pattern_reader) must have:
        'corners': numpy array (4, 2)
        'center':  (cx, cy)
        'area':    float
        'type':    'left', 'right', or 'unknown'

    Returns a list of valid gates. Each gate is:
        {
            'left':   left marker dict,
            'right':  right marker dict,
            'center': [cx, cy]  midpoint between the two markers,
            'width':  float     pixel distance between the two centers
        }
    """
    gates = []

    left_markers = [m for m in markers if m['type'] == 'left']
    right_markers = [m for m in markers if m['type'] == 'right']

    for left in left_markers:
        for right in right_markers:
            area_diff, angle, gate_width = gate_metrics(left, right)
            if area_diff > AREA_MATCH_THRESHOLD:
                continue
            if angle > ANGLE_MAX_DEGREES:
                continue

            gate_center = midpoint(left['center'], right['center'])

            gates.append({
                'left': left,
                'right': right,
                'center': gate_center,
                'width': gate_width,
                'area_diff': area_diff,
                'angle': angle,
                'score': gate_score(area_diff, angle, gate_width),
                'stale': False,
            })

    return gates


def best_gate(gates):
    """
    If multiple valid gates are found, pick the most relevant one.
    Strategy: widest gate = closest to camera = highest priority.
    """
    if not gates:
        return None

    return max(gates, key=lambda g: g.get('score', g['width']))
