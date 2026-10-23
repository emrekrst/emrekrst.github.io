import pathlib
import subprocess
import sys
import tempfile
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]
VISION = ROOT / "final_navigation/vision"
sys.path.insert(0, str(VISION))

class OfflineDemoTest(unittest.TestCase):
    def test_demo_detects_two_markers_gate_and_command(self):
        with tempfile.TemporaryDirectory() as directory:
            result = subprocess.run([sys.executable, str(VISION / "offline_demo.py"),
                                     "--output", directory], capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stderr)
            self.assertIn('"markers": 2', result.stdout)
            self.assertIn('"command": "go"', result.stdout)
            self.assertTrue((pathlib.Path(directory) / "gate-demo.png").is_file())

class VisionBehaviorTest(unittest.TestCase):
    def test_patterns_survive_rotation(self):
        import numpy as np
        from config import PATTERNS_PI
        from custom_marker_finder import classify_grid
        for kind, pattern in PATTERNS_PI.items():
            for turns in range(4):
                self.assertEqual(classify_grid(np.rot90(np.array(pattern), turns))[0], kind)

    def test_blank_frame_contains_no_gate(self):
        import numpy as np
        from custom_marker_finder import find_markers
        from gate_finder import find_gates
        markers = find_markers(frame=np.full((720,1280,3), 255, dtype=np.uint8), include_unknown=False)
        self.assertEqual(find_gates(markers), [])

    def test_gate_rejects_different_size_or_vertical_markers(self):
        from gate_finder import find_gates
        left = {"type":"left", "center":(100,200), "area":1600}
        self.assertEqual(find_gates([left, {"type":"right", "center":(500,200), "area":300}]), [])
        self.assertEqual(find_gates([left, {"type":"right", "center":(100,400), "area":1600}]), [])

    def test_dropout_hold_expires_and_recovers(self):
        from gate_tracker import GateTracker
        tracker = GateTracker(memory_frames=2, alpha=.5)
        self.assertIsNone(tracker.update(None))
        tracker.update({"center":[100,200],"width":300})
        self.assertEqual(tracker.update({"center":[200,200],"width":400})["center"], [150,200])
        self.assertTrue(tracker.update(None)["stale"])
        self.assertTrue(tracker.update(None)["stale"])
        self.assertIsNone(tracker.update(None))
        self.assertEqual(tracker.update({"center":[300,200],"width":100})["center"], [300,200])

    def test_steering_edges_center_and_letter_case(self):
        from path_finder import x_to_letter, get_command, gate_to_command
        from config import IMG_WIDTH
        self.assertEqual(x_to_letter(-10), 'A')
        self.assertEqual(x_to_letter(IMG_WIDTH+10, False), 'z')
        self.assertEqual(x_to_letter(IMG_WIDTH/2), 'N')
        self.assertEqual(get_command(IMG_WIDTH/2)[0], 'go')
        self.assertEqual(get_command(0)[0], 'strong_left')
        self.assertEqual(get_command(IMG_WIDTH)[0], 'strong_right')
        self.assertEqual(gate_to_command(None), (None,None))

if __name__ == "__main__":
    unittest.main()
