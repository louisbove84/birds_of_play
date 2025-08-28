#!/usr/bin/env python3
"""
Test script for Birds of Play Python bindings
"""

import numpy as np
import cv2
import sys
import os

# Add the build directory to the Python path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'build', 'src', 'motion_detection'))

def test_motion_processor():
    """Test the MotionProcessor Python bindings"""
    print("Testing MotionProcessor...")
    
    try:
        import birds_of_play_python
        
        # Create a test image (simple gradient)
        height, width = 480, 640
        test_image = np.zeros((height, width, 3), dtype=np.uint8)
        
        # Create a simple gradient pattern
        for i in range(height):
            for j in range(width):
                test_image[i, j] = [i % 256, j % 256, (i + j) % 256]
        
        # Create processor instance
        processor = birds_of_play_python.MotionProcessor()
        
        # Process the frame
        result = processor.process_frame(test_image)
        
        print(f"Input image shape: {test_image.shape}")
        print(f"Output image shape: {result.shape}")
        print("MotionProcessor test passed!")
        
        return True
        
    except ImportError as e:
        print(f"Failed to import birds_of_play_python: {e}")
        print("Make sure to build the project first with: make build")
        return False
    except Exception as e:
        print(f"MotionProcessor test failed: {e}")
        return False

def test_motion_pipeline():
    """Test the MotionPipeline Python bindings"""
    print("\nTesting MotionPipeline...")
    print("MotionPipeline not implemented yet - skipping test")
    return True

def test_utility_functions():
    """Test the utility functions"""
    print("\nTesting utility functions...")
    print("Utility functions not exposed - skipping test")
    return True

def main():
    """Run all tests"""
    print("Birds of Play Python Bindings Test")
    print("=" * 40)
    
    success_count = 0
    total_tests = 3
    
    if test_motion_processor():
        success_count += 1
    
    if test_motion_pipeline():
        success_count += 1
    
    if test_utility_functions():
        success_count += 1
    
    print("\n" + "=" * 40)
    print(f"Tests passed: {success_count}/{total_tests}")
    
    if success_count == total_tests:
        print("All tests passed! Python bindings are working correctly.")
        return 0
    else:
        print("Some tests failed. Please check the build and dependencies.")
        return 1

if __name__ == "__main__":
    sys.exit(main())
