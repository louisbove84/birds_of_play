#!/usr/bin/env python3
import cv2
import numpy as np

# Create a motion mask image (1920x1080)
mask = np.zeros((1080, 1920), dtype=np.uint8)

# Add some motion regions (white blobs on black background)
cv2.rectangle(mask, (200, 150), (400, 250), 255, -1)
cv2.rectangle(mask, (600, 400), (700, 500), 255, -1)
cv2.rectangle(mask, (800, 500), (850, 550), 255, -1)
cv2.rectangle(mask, (900, 600), (950, 650), 255, -1)
cv2.rectangle(mask, (1000, 700), (1050, 750), 255, -1)

# Add some noise
cv2.rectangle(mask, (300, 300), (320, 320), 255, -1)
cv2.rectangle(mask, (500, 600), (520, 620), 255, -1)

cv2.imwrite("motion_mask.png", mask)
print("Created motion_mask.png")
