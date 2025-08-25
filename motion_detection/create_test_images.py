#!/usr/bin/env python3
"""
Script to create test images for motion detection testing.
Creates synthetic images with moving objects to test the motion detection pipeline.
"""

import cv2
import numpy as np
import os

def create_test_images():
    """Create test images for motion detection testing."""
    
    # Create test_images directory
    test_dir = "test_images"
    if not os.path.exists(test_dir):
        os.makedirs(test_dir)
        print(f"Created directory: {test_dir}")
    
    # Image dimensions
    width, height = 1920, 1080
    
    # Create first frame (background)
    frame1 = np.ones((height, width, 3), dtype=np.uint8) * 128  # Gray background
    
    # Add some static elements
    cv2.rectangle(frame1, (100, 100), (300, 200), (64, 64, 64), -1)  # Static rectangle
    cv2.circle(frame1, (500, 300), 50, (64, 64, 64), -1)  # Static circle
    
    # Create second frame (with motion)
    frame2 = frame1.copy()
    
    # Add moving objects
    # Moving rectangle
    cv2.rectangle(frame2, (200, 150), (400, 250), (255, 255, 255), -1)
    
    # Moving circle
    cv2.circle(frame2, (600, 400), 60, (255, 255, 255), -1)
    
    # Moving small objects (simulating birds)
    cv2.rectangle(frame2, (800, 500), (850, 550), (255, 255, 255), -1)
    cv2.rectangle(frame2, (900, 600), (950, 650), (255, 255, 255), -1)
    cv2.rectangle(frame2, (1000, 700), (1050, 750), (255, 255, 255), -1)
    
    # Save images
    cv2.imwrite(os.path.join(test_dir, "test_image.jpg"), frame1)
    cv2.imwrite(os.path.join(test_dir, "test_image2.jpg"), frame2)
    
    print(f"Created test images:")
    print(f"  - {test_dir}/test_image.jpg (static background)")
    print(f"  - {test_dir}/test_image2.jpg (with motion)")

if __name__ == "__main__":
    create_test_images()
