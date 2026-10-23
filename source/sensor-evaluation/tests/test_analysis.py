from pathlib import Path
import importlib.util
import unittest
ROOT = Path(__file__).resolve().parents[1]
class AnalysisTest(unittest.TestCase):
    def test_sentinels_are_failures_and_not_distances(self):
        self.assertTrue((ROOT/'analyze.py').exists(), 'Missing reproducible invalid-reading rejection')
        spec=importlib.util.spec_from_file_location('analysis', ROOT/'analyze.py')
        m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
        self.assertEqual(m.summarize('ToF',[819,819,819,819,819]), (0,5,None,None))
        self.assertEqual(m.summarize('Ultrasonic',[-.02]*5), (0,5,None,None))
        self.assertEqual(m.summarize('ToF',[10,12,819]), (2,1,11,1))
if __name__=='__main__':unittest.main()
