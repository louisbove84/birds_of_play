import unittest

class TestSample(unittest.TestCase):
    def test_basic_assertion(self):
        self.assertEqual(2 + 2, 4)

if __name__ == '__main__':
    unittest.main()
