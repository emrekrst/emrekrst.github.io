import math
import numpy as np

def shoelace_area(corners):
    """
    Calculate area of polygon using Shoelace Formula (Gauss's Area Formula)
    Input: corners = [[x1,y1], [x2,y2], [x3,y3], [x4,y4]] (list of 4 points)
    Output: area in pixels (float)
    
    Formula: Area = 0.5 * |sum of (x[i]*y[i+1] - x[i+1]*y[i])|
    Like tying shoelaces with coordinates!
    """
    corners = np.array(corners)
    n = len(corners)
    
    # Shoelace calculation
    area = 0.0
    for i in range(n):
        j = (i + 1) % n  # Wrap around to first point
        area += corners[i][0] * corners[j][1]  # x[i] * y[j]
        area -= corners[j][0] * corners[i][1]  # x[j] * y[i]
    
    area = abs(area) / 2.0
    return area


def distance(point1, point2):
    """
    Euclidean distance between two points
    
    Input: point1 = [x1, y1], point2 = [x2, y2]
    Output: distance in pixels (float)
    
    Formula: sqrt((x2-x1)^2 + (y2-y1)^2) - Pythagorean theorem
    """
    x1, y1 = point1
    x2, y2 = point2
    dist = math.sqrt((x2 - x1)**2 + (y2 - y1)**2)
    return dist


def angle_between(point1, point2):
    """
    Calculate angle between two points relative to horizontal X-axis
    
    Input: point1 = [x1, y1] (left marker), point2 = [x2, y2] (right marker)
    Output: angle in degrees (0-90)
    
    Formula: arctan(|y2-y1| / |x2-x1|) converted to degrees
    """
    x1, y1 = point1
    x2, y2 = point2
    
    dx = abs(x2 - x1)
    dy = abs(y2 - y1)
    
    # Avoid division by zero
    if dx == 0:
        return 90.0  # Vertical line
    
    angle_rad = math.atan(dy / dx)
    angle_deg = math.degrees(angle_rad)
    return angle_deg


def midpoint(point1, point2):
    """
    Calculate midpoint (average) of two points
    Used to find gate center between left and right markers
    
    Input: point1 = [x1, y1], point2 = [x2, y2]
    Output: [x_mid, y_mid] (center point)
    
    Formula: [(x1+x2)/2, (y1+y2)/2]
    """
    x1, y1 = point1
    x2, y2 = point2
    mid_x = (x1 + x2) / 2.0
    mid_y = (y1 + y2) / 2.0
    return [mid_x, mid_y]


def point_in_bounds(point, img_width, img_height):
    """
    Check if point is inside image boundaries
    Prevents processing markers outside frame
    
    Input: point = [x, y], img_width, img_height (from config)
    Output: True if inside, False if outside
    """
    x, y = point
    if 0 <= x < img_width and 0 <= y < img_height:
        return True
    return False
