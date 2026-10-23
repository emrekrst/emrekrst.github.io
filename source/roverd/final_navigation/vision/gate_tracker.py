# gate_tracker.py - Smooth gate position and bridge short detection dropouts.

from config import GATE_MEMORY_FRAMES, GATE_SMOOTHING_ALPHA


def lerp(a, b, alpha):
    return (1.0 - alpha) * float(a) + alpha * float(b)


def blend_point(old, new, alpha):
    return [
        lerp(old[0], new[0], alpha),
        lerp(old[1], new[1], alpha),
    ]


class GateTracker:
    def __init__(self, memory_frames=GATE_MEMORY_FRAMES, alpha=GATE_SMOOTHING_ALPHA):
        self.memory_frames = memory_frames
        self.alpha = alpha
        self.last_gate = None
        self.missed_frames = 0

    def reset(self):
        self.last_gate = None
        self.missed_frames = 0

    def update(self, gate):
        if gate is not None:
            self.missed_frames = 0
            if self.last_gate is None:
                self.last_gate = dict(gate)
            else:
                smoothed = dict(gate)
                smoothed['center'] = blend_point(self.last_gate['center'], gate['center'], self.alpha)
                smoothed['width'] = lerp(self.last_gate['width'], gate['width'], self.alpha)
                smoothed['stale'] = False
                self.last_gate = smoothed
            return self.last_gate

        if self.last_gate is None:
            return None

        self.missed_frames += 1
        if self.missed_frames > self.memory_frames:
            self.reset()
            return None

        held = dict(self.last_gate)
        held['stale'] = True
        held['missed_frames'] = self.missed_frames
        return held
