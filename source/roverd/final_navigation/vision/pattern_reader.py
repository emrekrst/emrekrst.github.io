# pattern_reader.py - Read and identify marker patterns
# Takes a detected square, unwarps it via perspective transform,
# reads the inner NxN binary grid, and matches against known patterns.

import numpy as np
import cv2
from config import PATTERNS

# Grid size is read dynamically from config (currently 4x4)
GRID_SIZE = len(list(PATTERNS.values())[0])

# Size of the normalised patch we warp each marker into
# 80px: border=16px each side, inner=48px, cell=12px per side
PATCH_SIZE = 80

# Black border takes up 20% on each side → inner region is 60% of patch
BORDER_FRACTION = 0.2


def warp_to_patch(frame, corners):
    """
    Perspective-warp the detected square into a flat PATCH_SIZE x PATCH_SIZE image.

    corners: numpy array (4, 2), ordered [TL, TR, BR, BL]
    Returns: warped patch (grayscale)
    """
    dst = np.array([
        [0,             0],
        [PATCH_SIZE - 1, 0],
        [PATCH_SIZE - 1, PATCH_SIZE - 1],
        [0,             PATCH_SIZE - 1]
    ], dtype=np.float32)

    src = corners.astype(np.float32)
    H = cv2.getPerspectiveTransform(src, dst)
    patch = cv2.warpPerspective(frame, H, (PATCH_SIZE, PATCH_SIZE))

    # Convert to grayscale for thresholding
    if len(patch.shape) == 3:
        patch = cv2.cvtColor(patch, cv2.COLOR_BGR2GRAY)

    return patch


def extract_grid(patch):
    """
    Convert the warped patch into an NxN binary grid (N = GRID_SIZE).

    Returns: numpy array (N, N) where 1 = black cell, 0 = white cell
    """
    # Otsu threshold → clean black/white image
    _, binary = cv2.threshold(patch, 0, 255, cv2.THRESH_BINARY + cv2.THRESH_OTSU)

    # Crop out the black border to get the inner pattern region
    border = int(PATCH_SIZE * BORDER_FRACTION)
    inner = binary[border:PATCH_SIZE - border, border:PATCH_SIZE - border]

    cell_size = inner.shape[0] // GRID_SIZE
    grid = np.zeros((GRID_SIZE, GRID_SIZE), dtype=int)

    for row in range(GRID_SIZE):
        for col in range(GRID_SIZE):
            cell = inner[row * cell_size:(row + 1) * cell_size,
                         col * cell_size:(col + 1) * cell_size]
            # Black pixels = 0 in binary image → mean < 127 means mostly black
            grid[row][col] = 1 if np.mean(cell) < 127 else 0

    return grid


def rotate_grid_90(grid):
    """Rotate grid 90 degrees clockwise."""
    return np.rot90(grid, k=-1)


def read_pattern(frame, corners):
    """
    Main function: identify which marker type a detected square is.

    Args:
        frame:   full camera frame (BGR or grayscale)
        corners: numpy array (4, 2), corners ordered [TL, TR, BR, BL]

    Returns:
        marker_type: 'left', 'right', or 'unknown'
        grid:        the NxN binary grid that was read (for debugging)
    """
    # 1. Unwarp the square into a flat patch
    patch = warp_to_patch(frame, corners)

    # 2. Read the NxN binary grid from the inner region
    grid = extract_grid(patch)

    # 3. Try all 4 rotations against every known pattern
    rotated = grid.copy()
    for _ in range(4):
        for name, pattern in PATTERNS.items():
            if np.array_equal(rotated, np.array(pattern)):
                return name, grid
        rotated = rotate_grid_90(rotated)

    return 'unknown', grid
