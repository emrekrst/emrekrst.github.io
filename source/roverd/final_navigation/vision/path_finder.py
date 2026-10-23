# path_finder.py - Decide steering direction from gate center position
# Divides the frame into 5 zones and maps gate center x to a command

from config import IMG_WIDTH, STEER_ZONE_LEFT, STEER_ZONE_RIGHT, LED_PINS, LETTER_NUM_ZONES

# Split each side further for fine-grained steering
# Frame: |--SL--|--LL--|----GO----|--LR--|--SR--|
ZONE_STRONG_LEFT  = STEER_ZONE_LEFT / 2          # 0  – 106
ZONE_LIGHT_LEFT   = STEER_ZONE_LEFT               # 107 – 213
ZONE_GO_LEFT      = STEER_ZONE_LEFT               # 213 – 426  (centre zone)
ZONE_LIGHT_RIGHT  = STEER_ZONE_RIGHT              # 426 – 533
ZONE_STRONG_RIGHT = STEER_ZONE_RIGHT + (IMG_WIDTH - STEER_ZONE_RIGHT) / 2  # 533 – 640


def x_to_letter(gate_center_x, uppercase=True):
    """
    Map gate center X position (0 to IMG_WIDTH) to a letter A-Z or a-z.

    Frame divided into 26 equal zones (~24.6px each):
      A = far left, M/N = center, Z = far right.

    Args:
        gate_center_x: pixel X coordinate of gate midpoint
        uppercase: True for odd gates (1,3,5...), False for even gates (2,4,6...)

    Returns:
        Single character, e.g. 'A', 'N', 'z'
    """
    clamped_x = max(0.0, min(float(gate_center_x), float(IMG_WIDTH)))
    zone_width = IMG_WIDTH / LETTER_NUM_ZONES
    zone_index = int(clamped_x / zone_width)
    if zone_index >= LETTER_NUM_ZONES:
        zone_index = LETTER_NUM_ZONES - 1

    if uppercase:
        return chr(ord('A') + zone_index)
    else:
        return chr(ord('a') + zone_index)


def get_command(gate_center_x):
    """
    Map gate centre x position to a steering command.

    Args:
        gate_center_x: x pixel coordinate of the gate midpoint

    Returns:
        command: one of 'strong_left', 'light_left', 'go',
                         'light_right', 'strong_right'
        led_pin: GPIO pin number to activate (from config)
    """
    if gate_center_x < ZONE_STRONG_LEFT:
        command = 'strong_left'
    elif gate_center_x < ZONE_LIGHT_LEFT:
        command = 'light_left'
    elif gate_center_x <= ZONE_LIGHT_RIGHT:
        command = 'go'
    elif gate_center_x <= ZONE_STRONG_RIGHT:
        command = 'light_right'
    else:
        command = 'strong_right'

    return command, LED_PINS[command]


def gate_to_command(gate):
    """
    Convenience wrapper: takes a gate dict from gate_finder and returns command.

    Args:
        gate: dict with 'center' key → [cx, cy]

    Returns:
        command, led_pin  (or None, None if gate is None)
    """
    if gate is None:
        return None, None

    gate_center_x = gate['center'][0]
    return get_command(gate_center_x)
