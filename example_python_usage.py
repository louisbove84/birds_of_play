#!/usr/bin/env python3
"""
Example usage of Birds of Play - runs the C++ motion detection executable
"""

import subprocess
import sys
import os
import time

def check_build():
    """Check if the C++ executable is built"""
    executable_path = os.path.join("build", "birds_of_play")
    if not os.path.exists(executable_path):
        print(f"❌ C++ executable not found at: {executable_path}")
        print("Please build the project first:")
        print("  mkdir -p build && cd build")
        print("  cmake .. -DBUILD_TESTING=ON")
        print("  make -j$(nproc)")
        return False
    return True

def run_motion_detection_demo():
    """Run the C++ motion detection demo with webcam"""
    print("🐦 Birds of Play - C++ Motion Detection Demo")
    print("=" * 50)
    
    if not check_build():
        return False
    
    executable_path = os.path.join("build", "birds_of_play")
    config_path = os.path.join("src", "motion_detection", "config.yaml")
    
    print(f"📹 Starting motion detection with webcam...")
    print(f"🔧 Using config: {config_path}")
    print(f"⚙️  Executable: {executable_path}")
    print("\n⌨️  Controls:")
    print("   'q' or ESC - Quit application")
    print("   's' - Save current frame with detections")
    print("\n🔍 Watch for:")
    print("   🟢 Green/Blue/Red boxes - Individual motion detections")
    print("   ⬜ White boxes - Consolidated motion regions")
    print("\n" + "=" * 50)
    
    try:
        # Run the C++ executable
        cmd = [executable_path, config_path]
        print(f"🚀 Running: {' '.join(cmd)}")
        
        # Start the process
        process = subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            universal_newlines=True,
            bufsize=1
        )
        
        # Print output in real-time
        print("\n📺 Motion detection window should open...")
        print("📊 Live output:")
        print("-" * 30)
        
        while True:
            output = process.stdout.readline()
            if output == '' and process.poll() is not None:
                break
            if output:
                print(output.strip())
        
        # Wait for process to finish
        return_code = process.poll()
        
        if return_code == 0:
            print("\n✅ Motion detection demo completed successfully!")
        else:
            print(f"\n❌ Motion detection demo exited with code: {return_code}")
            
    except KeyboardInterrupt:
        print("\n⏹️  Demo interrupted by user")
        if 'process' in locals():
            process.terminate()
            process.wait()
    except Exception as e:
        print(f"\n❌ Error running motion detection demo: {e}")
        return False
    
    return True

def run_with_video_file(video_path):
    """Run motion detection on a video file instead of webcam"""
    print(f"🎬 Birds of Play - Motion Detection on Video: {video_path}")
    print("=" * 50)
    
    if not check_build():
        return False
    
    if not os.path.exists(video_path):
        print(f"❌ Video file not found: {video_path}")
        return False
    
    executable_path = os.path.join("build", "birds_of_play")
    config_path = os.path.join("src", "motion_detection", "config.yaml")
    
    print(f"📹 Processing video: {video_path}")
    print(f"🔧 Using config: {config_path}")
    
    try:
        # Run the C++ executable with video file
        cmd = [executable_path, config_path, video_path]
        print(f"🚀 Running: {' '.join(cmd)}")
        
        process = subprocess.Popen(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            universal_newlines=True,
            bufsize=1
        )
        
        print("\n📊 Processing output:")
        print("-" * 30)
        
        while True:
            output = process.stdout.readline()
            if output == '' and process.poll() is not None:
                break
            if output:
                print(output.strip())
        
        return_code = process.poll()
        
        if return_code == 0:
            print("\n✅ Video processing completed successfully!")
        else:
            print(f"\n❌ Video processing exited with code: {return_code}")
            
    except KeyboardInterrupt:
        print("\n⏹️  Processing interrupted by user")
        if 'process' in locals():
            process.terminate()
            process.wait()
    except Exception as e:
        print(f"\n❌ Error processing video: {e}")
        return False
    
    return True

def main():
    """Main function"""
    print("🐦 Birds of Play - C++ Motion Detection Demo Launcher")
    print("=" * 60)
    
    print("Choose an option:")
    print("1. 🎥 Run motion detection with webcam (live)")
    print("2. 🎬 Run motion detection on video file")
    print("3. 🚪 Exit")
    
    while True:
        choice = input("\nEnter your choice (1-3): ").strip()
        
        if choice == "1":
            run_motion_detection_demo()
            break
        elif choice == "2":
            video_path = input("Enter path to video file: ").strip()
            if video_path:
                run_with_video_file(video_path)
            break
        elif choice == "3":
            print("👋 Goodbye!")
            break
        else:
            print("❌ Invalid choice. Please enter 1, 2, or 3.")

if __name__ == "__main__":
    main()
