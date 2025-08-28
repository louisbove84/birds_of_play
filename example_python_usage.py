#!/usr/bin/env python3
"""
Example usage of Birds of Play Python bindings
"""

import numpy as np
import cv2
import sys
import os

# Add the build directory to the Python path
sys.path.insert(0, os.path.join(os.path.dirname(__file__), 'build', 'src', 'motion_detection'))

def create_test_video():
    """Create a simple test video with moving objects"""
    print("Creating test video...")
    
    # Create a video writer
    fourcc = cv2.VideoWriter_fourcc(*'mp4v')
    out = cv2.VideoWriter('test_video.mp4', fourcc, 30.0, (640, 480))
    
    # Create frames with moving objects
    for frame_num in range(100):  # 3.3 seconds at 30 fps
        # Create a black frame
        frame = np.zeros((480, 640, 3), dtype=np.uint8)
        
        # Add a moving rectangle
        x = int(50 + frame_num * 2)  # Move right
        y = int(200 + 20 * np.sin(frame_num * 0.1))  # Oscillate up and down
        cv2.rectangle(frame, (x, y), (x + 50, y + 50), (255, 255, 255), -1)
        
        # Add a moving circle
        circle_x = int(400 - frame_num * 1.5)  # Move left
        circle_y = int(300 + 30 * np.cos(frame_num * 0.15))  # Oscillate
        cv2.circle(frame, (circle_x, circle_y), 30, (128, 128, 128), -1)
        
        # Add some noise
        noise = np.random.randint(0, 30, frame.shape, dtype=np.uint8)
        frame = cv2.add(frame, noise)
        
        out.write(frame)
    
    out.release()
    print("Test video created: test_video.mp4")

def process_video_with_motion_detection():
    """Process the test video using the Python bindings"""
    print("\nProcessing video with motion detection...")
    
    try:
        import birds_of_play_python
        
        # Create motion processor
        processor = birds_of_play_python.MotionProcessor()
        
        # Open the test video
        cap = cv2.VideoCapture('test_video.mp4')
        
        if not cap.isOpened():
            print("Error: Could not open test video")
            return
        
        # Create output video writer
        fourcc = cv2.VideoWriter_fourcc(*'mp4v')
        out = cv2.VideoWriter('motion_detection_output.mp4', fourcc, 30.0, (640, 480))
        
        frame_count = 0
        while True:
            ret, frame = cap.read()
            if not ret:
                break
            
            # Process frame with motion detection
            processed_frame = processor.process_frame(frame)
            
            # Convert back to BGR for video writing
            if len(processed_frame.shape) == 3 and processed_frame.shape[2] == 1:
                # Convert grayscale to BGR
                processed_frame = cv2.cvtColor(processed_frame, cv2.COLOR_GRAY2BGR)
            
            # Write the processed frame
            out.write(processed_frame)
            
            frame_count += 1
            if frame_count % 30 == 0:  # Print progress every second
                print(f"Processed {frame_count} frames...")
        
        cap.release()
        out.release()
        print(f"Motion detection processing complete. Output saved to: motion_detection_output.mp4")
        
    except ImportError as e:
        print(f"Failed to import birds_of_play_python: {e}")
        print("Make sure to build the project first with: cd build && make")
    except Exception as e:
        print(f"Error during video processing: {e}")

def interactive_demo():
    """Interactive demo using webcam"""
    print("\nStarting interactive demo (press 'q' to quit)...")
    
    try:
        import birds_of_play_python
        
        # Create motion processor
        processor = birds_of_play_python.MotionProcessor()
        
        # Open webcam
        cap = cv2.VideoCapture(0)
        
        if not cap.isOpened():
            print("Error: Could not open webcam")
            return
        
        print("Webcam opened successfully. Press 'q' to quit.")
        
        while True:
            ret, frame = cap.read()
            if not ret:
                print("Error: Could not read frame from webcam")
                break
            
            # Process frame with motion detection
            processed_frame = processor.process_frame(frame)
            
            # Convert back to BGR for display
            if len(processed_frame.shape) == 3 and processed_frame.shape[2] == 1:
                # Convert grayscale to BGR
                processed_frame = cv2.cvtColor(processed_frame, cv2.COLOR_GRAY2BGR)
            
            # Display both original and processed frames
            combined = np.hstack([frame, processed_frame])
            cv2.imshow('Birds of Play - Motion Detection (Original | Processed)', combined)
            
            # Check for quit key
            if cv2.waitKey(1) & 0xFF == ord('q'):
                break
        
        cap.release()
        cv2.destroyAllWindows()
        print("Interactive demo ended.")
        
    except ImportError as e:
        print(f"Failed to import birds_of_play_python: {e}")
        print("Make sure to build the project first with: cd build && make")
    except Exception as e:
        print(f"Error during interactive demo: {e}")

def main():
    """Main function"""
    print("Birds of Play Python Bindings Example")
    print("=" * 40)
    
    # Create test video
    create_test_video()
    
    # Process video with motion detection
    process_video_with_motion_detection()
    
    # Ask user if they want to try interactive demo
    print("\n" + "=" * 40)
    response = input("Would you like to try the interactive webcam demo? (y/n): ")
    
    if response.lower() in ['y', 'yes']:
        interactive_demo()
    else:
        print("Demo complete! Check the output files:")
        print("- test_video.mp4: Input test video")
        print("- motion_detection_output.mp4: Processed video with motion detection")

if __name__ == "__main__":
    main()
