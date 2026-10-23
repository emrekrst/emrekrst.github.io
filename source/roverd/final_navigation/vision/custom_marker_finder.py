# custom_marker_finder.py - Robust detector for the project's custom 4x4 markers
#
# This keeps the old marker contract:
#   square contour -> perspective warp -> 4x4 grid -> left/right type
# but packages it as one detector stage and removes duplicate contour hits.

import cv2
import numpy as np

import config
from pattern_reader import extract_grid, rotate_grid_90, warp_to_patch
from square_finder import find_squares


DEFAULT_MAX_HAMMING = getattr(config, 'CUSTOM_MARKER_MAX_HAMMING', 2)
DEFAULT_DUPLICATE_CENTER_FRACTION = getattr(config, 'CUSTOM_MARKER_DUPLICATE_CENTER_FRACTION', 0.35)


def hamming_distance(a, b):
    return int(np.count_nonzero(np.asarray(a, dtype=int) != np.asarray(b, dtype=int)))


def classify_grid(grid, max_hamming=DEFAULT_MAX_HAMMING):
    """
    Match a read grid against configured patterns, trying all rotations.

    Returns:
        marker_type: 'left', 'right', or 'unknown'
        best_error:  number of bit differences from the best pattern
        rotation:    rotation index that matched best
    """
    patterns = getattr(config, 'PATTERNS', {})
    best_type = 'unknown'
    best_error = grid.size + 1
    best_rotation = 0

    rotated = grid.copy()
    for rotation in range(4):
        for marker_type, pattern in patterns.items():
            error = hamming_distance(rotated, pattern)
            if error < best_error:
                best_type = marker_type
                best_error = error
                best_rotation = rotation
        rotated = rotate_grid_90(rotated)

    if best_error > max_hamming:
        return 'unknown', best_error, best_rotation
    return best_type, best_error, best_rotation


def marker_scale(marker):
    return float(np.sqrt(max(marker.get('area', 0.0), 1.0)))


def is_duplicate(a, b, center_fraction=DEFAULT_DUPLICATE_CENTER_FRACTION):
    ax, ay = a['center']
    bx, by = b['center']
    center_distance = float(np.hypot(ax - bx, ay - by))
    scale = max(marker_scale(a), marker_scale(b))
    return center_distance < center_fraction * scale


def marker_rank(marker):
    # Known marker beats unknown; lower grid error beats higher; larger area is
    # usually the outer border and is more stable than an inner duplicate.
    known_score = 0 if marker['type'] != 'unknown' else 1
    return (known_score, marker.get('match_error', 99), -marker.get('area', 0.0))


def suppress_duplicates(markers):
    kept = []
    for marker in sorted(markers, key=marker_rank):
        if any(is_duplicate(marker, existing) for existing in kept):
            continue
        kept.append(marker)
    return kept


def find_markers(gray=None, frame=None, min_area=1000, max_area=50000, include_unknown=True, **kwargs):
    """
    Detect and classify custom grid markers.

    Each returned marker has the same shape downstream code expects:
        corners, center, area, type

    Extra debug fields:
        grid, match_error, rotation
    """
    if gray is None:
        if frame is None:
            print('ERROR: Need grayscale image or BGR frame')
            return []
        gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)

    source = frame if frame is not None else gray
    candidates = find_squares(gray=gray, min_area=min_area, max_area=max_area, **kwargs)
    markers = []

    for candidate in candidates:
        patch = warp_to_patch(source, candidate['corners'])
        grid = extract_grid(patch)
        marker_type, match_error, rotation = classify_grid(grid)
        if marker_type == 'unknown' and not include_unknown:
            continue

        marker = dict(candidate)
        marker.update({
            'type': marker_type,
            'grid': grid,
            'match_error': match_error,
            'rotation': rotation,
        })
        markers.append(marker)

    return suppress_duplicates(markers)
