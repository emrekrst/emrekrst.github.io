# square_finder.py - Direct Contour Detection (quads.py approach)
# Simple and robust: adaptive threshold + contour detection
# No Hough lines needed - direct detection of square markers

import numpy as np
import cv2
import math
import sys
sys.path.append('..')

from math_utils import shoelace_area, distance


def order_corners(corners):
    """
    Order 4 corners consistently: top-left, top-right, bottom-right, bottom-left
    """
    pts = np.array(corners, dtype=np.float32)
    rect = np.zeros((4, 2), dtype=np.float32)
    
    s = pts.sum(axis=1)
    rect[0] = pts[np.argmin(s)]      # top-left
    rect[2] = pts[np.argmax(s)]      # bottom-right
    
    diff = np.diff(pts, axis=1)
    rect[1] = pts[np.argmin(diff)]   # top-right
    rect[3] = pts[np.argmax(diff)]   # bottom-left
    
    return rect


def find_squares_direct(gray, min_area=1000, max_area=50000):
    """
    Direct contour detection using adaptive threshold (quads.py approach)
    Simple and robust - no Hough lines needed
    """
    try:
        # Adaptive threshold finds local contrast changes = marker borders
        blur = cv2.GaussianBlur(gray, (5, 5), 0)
        thresh = cv2.adaptiveThreshold(
            blur, 255,
            cv2.ADAPTIVE_THRESH_GAUSSIAN_C,
            cv2.THRESH_BINARY_INV,
            11, 2
        )

        # Light morphology to clean up noise
        kernel = np.ones((3, 3), np.uint8)
        thresh = cv2.morphologyEx(thresh, cv2.MORPH_CLOSE, kernel, iterations=1)

        # Find contours
        contours, _ = cv2.findContours(thresh, cv2.RETR_TREE, cv2.CHAIN_APPROX_SIMPLE)

    except cv2.error as e:
        print(f"OpenCV error in preprocessing: {e}")
        return []
    except Exception as e:
        print(f"Unexpected error in preprocessing: {e}")
        return []

    markers = []
    
    for contour in contours:
        try:
            area = cv2.contourArea(contour)

            
            if area < min_area or area > max_area:
                continue

            perimeter = cv2.arcLength(contour, True)
            approx = cv2.approxPolyDP(contour, 0.04 * perimeter, True)

            # Must be a quadrilateral
            if len(approx) != 4:
                continue

            # Must be convex
            if not cv2.isContourConvex(approx):
                continue

            corners = approx.reshape(-1, 2).astype(np.float32)
            ordered = order_corners(corners)

            # Check aspect ratio using minimum area rectangle
            rect = cv2.minAreaRect(corners)
            w, h = rect[1]
            if w < 20 or h < 20:
                continue

            ratio = max(w, h) / min(w, h)

            # ArUco markers are SQUARE - ratio should be close to 1.0
            if ratio > 1.5:
                continue

            # Check side lengths for squareness
            side_lengths = []
            for i in range(4):
                p1 = ordered[i]
                p2 = ordered[(i + 1) % 4]
                length = distance(p1.tolist(), p2.tolist())
                side_lengths.append(length)

            side_ratio = max(side_lengths) / (min(side_lengths) + 1e-6)
            if side_ratio > 1.6:
                continue

            precise_area = shoelace_area(ordered.tolist())
            cx = np.mean(ordered[:, 0])
            cy = np.mean(ordered[:, 1])

            markers.append({
                'corners': ordered,
                'center': (cx, cy),
                'area': precise_area
            })
        except Exception as e:
            # Skip problematic contours
            continue
    
    return markers


def find_squares(gray=None, img_width=None, img_height=None, min_area=1000, max_area=50000, **kwargs):
    """
    Main function: Direct contour detection (quads.py approach)
    
    Simple and robust - no Hough lines needed
    Uses adaptive thresholding to find square markers
    """
    if gray is None:
        print("ERROR: Need grayscale image")
        return []
    
    markers = find_squares_direct(gray, min_area, max_area)
    return markers

