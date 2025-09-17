"""
Performance Test: Base64 vs File Storage

This script compares the performance of storing images in MongoDB
as base64 vs storing them as local files with metadata in MongoDB.
"""

import os
import sys
import time
import uuid
import logging
from pathlib import Path
from typing import List, Dict, Any
import numpy as np
import cv2

# Add the src directory to the Python path
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '../..')))

from mongodb.database_manager import DatabaseManager
from mongodb.frame_database import FrameDatabase  # Old version
from mongodb.frame_database_v2 import FrameDatabaseV2  # New version


class PerformanceTester:
    """Tests performance differences between storage methods."""
    
    def __init__(self):
        """Initialize the performance tester."""
        self.logger = logging.getLogger(__name__)
        
        # Initialize database connections
        self.db_manager = DatabaseManager()
        if not self.db_manager.connect():
            raise RuntimeError("Failed to connect to MongoDB")
        
        # Initialize both frame databases
        self.old_frame_db = FrameDatabase(self.db_manager)
        self.new_frame_db = FrameDatabaseV2(self.db_manager, "data/performance_test")
        
        # Test data
        self.test_frames = []
        self.test_uuids = []
        
    def generate_test_frames(self, count: int = 100) -> List[np.ndarray]:
        """
        Generate test frames with different sizes and content.
        
        Args:
            count: Number of test frames to generate
            
        Returns:
            List of test frames
        """
        self.logger.info(f"Generating {count} test frames...")
        
        frames = []
        for i in range(count):
            # Create frames of different sizes
            if i % 3 == 0:
                # Small frame (320x240)
                frame = np.random.randint(0, 255, (240, 320, 3), dtype=np.uint8)
            elif i % 3 == 1:
                # Medium frame (640x480)
                frame = np.random.randint(0, 255, (480, 640, 3), dtype=np.uint8)
            else:
                # Large frame (1280x720)
                frame = np.random.randint(0, 255, (720, 1280, 3), dtype=np.uint8)
            
            # Add some structure to make it more realistic
            cv2.rectangle(frame, (10, 10), (50, 50), (255, 0, 0), 2)
            cv2.circle(frame, (100, 100), 30, (0, 255, 0), -1)
            cv2.putText(frame, f"Frame {i}", (200, 200), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 0, 255), 2)
            
            frames.append(frame)
        
        self.test_frames = frames
        self.logger.info(f"Generated {len(frames)} test frames")
        return frames
    
    def test_base64_storage(self) -> Dict[str, Any]:
        """
        Test storing frames using base64 in MongoDB.
        
        Returns:
            Performance metrics
        """
        self.logger.info("Testing base64 storage...")
        
        start_time = time.time()
        uuids = []
        
        for i, frame in enumerate(self.test_frames):
            metadata = {
                "test_id": f"base64_test_{i}",
                "frame_size": frame.shape,
                "timestamp": time.time()
            }
            
            frame_uuid = self.old_frame_db.save_frame(frame, metadata)
            if frame_uuid:
                uuids.append(frame_uuid)
        
        save_time = time.time() - start_time
        
        # Test retrieval
        start_time = time.time()
        retrieved_frames = []
        
        for frame_uuid in uuids[:10]:  # Test retrieval of first 10
            frame = self.old_frame_db.get_frame(frame_uuid)
            if frame is not None:
                retrieved_frames.append(frame)
        
        retrieval_time = time.time() - start_time
        
        # Test metadata retrieval
        start_time = time.time()
        metadata_list = []
        
        for frame_uuid in uuids[:10]:
            metadata = self.old_frame_db.get_frame_metadata(frame_uuid)
            if metadata:
                metadata_list.append(metadata)
        
        metadata_time = time.time() - start_time
        
        return {
            "method": "base64",
            "frames_saved": len(uuids),
            "save_time": save_time,
            "save_rate": len(uuids) / save_time,
            "retrieval_time": retrieval_time,
            "retrieval_rate": len(retrieved_frames) / retrieval_time,
            "metadata_time": metadata_time,
            "metadata_rate": len(metadata_list) / metadata_time,
            "uuids": uuids
        }
    
    def test_file_storage(self) -> Dict[str, Any]:
        """
        Test storing frames using local files with MongoDB metadata.
        
        Returns:
            Performance metrics
        """
        self.logger.info("Testing file storage...")
        
        start_time = time.time()
        uuids = []
        
        for i, frame in enumerate(self.test_frames):
            metadata = {
                "test_id": f"file_test_{i}",
                "frame_size": frame.shape,
                "timestamp": time.time()
            }
            
            frame_uuid = self.new_frame_db.save_frame(frame, metadata)
            if frame_uuid:
                uuids.append(frame_uuid)
        
        save_time = time.time() - start_time
        
        # Test retrieval
        start_time = time.time()
        retrieved_frames = []
        
        for frame_uuid in uuids[:10]:  # Test retrieval of first 10
            frame = self.new_frame_db.get_frame(frame_uuid)
            if frame is not None:
                retrieved_frames.append(frame)
        
        retrieval_time = time.time() - start_time
        
        # Test metadata retrieval
        start_time = time.time()
        metadata_list = []
        
        for frame_uuid in uuids[:10]:
            metadata = self.new_frame_db.get_frame_metadata(frame_uuid)
            if metadata:
                metadata_list.append(metadata)
        
        metadata_time = time.time() - start_time
        
        return {
            "method": "file_storage",
            "frames_saved": len(uuids),
            "save_time": save_time,
            "save_rate": len(uuids) / save_time,
            "retrieval_time": retrieval_time,
            "retrieval_rate": len(retrieved_frames) / retrieval_time,
            "metadata_time": metadata_time,
            "metadata_rate": len(metadata_list) / metadata_time,
            "uuids": uuids
        }
    
    def test_file_storage_with_original(self) -> Dict[str, Any]:
        """
        Test storing both original and processed frames using file storage.
        
        Returns:
            Performance metrics
        """
        self.logger.info("Testing file storage with original frames...")
        
        start_time = time.time()
        uuids = []
        
        for i, frame in enumerate(self.test_frames):
            # Create a "processed" version with some modifications
            processed_frame = frame.copy()
            cv2.rectangle(processed_frame, (0, 0), (50, 50), (255, 255, 0), 3)
            
            metadata = {
                "test_id": f"file_original_test_{i}",
                "frame_size": frame.shape,
                "timestamp": time.time()
            }
            
            frame_uuid = self.new_frame_db.save_frame_with_original(frame, processed_frame, metadata)
            if frame_uuid:
                uuids.append(frame_uuid)
        
        save_time = time.time() - start_time
        
        # Test retrieval of both types
        start_time = time.time()
        original_frames = []
        processed_frames = []
        
        for frame_uuid in uuids[:10]:  # Test retrieval of first 10
            original_frame = self.new_frame_db.get_frame(frame_uuid, "original")
            processed_frame = self.new_frame_db.get_frame(frame_uuid, "processed")
            
            if original_frame is not None:
                original_frames.append(original_frame)
            if processed_frame is not None:
                processed_frames.append(processed_frame)
        
        retrieval_time = time.time() - start_time
        
        return {
            "method": "file_storage_with_original",
            "frames_saved": len(uuids),
            "save_time": save_time,
            "save_rate": len(uuids) / save_time,
            "retrieval_time": retrieval_time,
            "retrieval_rate": (len(original_frames) + len(processed_frames)) / retrieval_time,
            "original_frames_retrieved": len(original_frames),
            "processed_frames_retrieved": len(processed_frames),
            "uuids": uuids
        }
    
    def cleanup_test_data(self, uuids: List[str], method: str):
        """
        Clean up test data.
        
        Args:
            uuids: List of frame UUIDs to delete
            method: Storage method used
        """
        self.logger.info(f"Cleaning up {method} test data...")
        
        for frame_uuid in uuids:
            if method == "base64":
                self.old_frame_db.delete_frame(frame_uuid)
            else:
                self.new_frame_db.delete_frame(frame_uuid)
        
        self.logger.info(f"Cleaned up {len(uuids)} frames")
    
    def run_performance_test(self, frame_count: int = 100) -> Dict[str, Any]:
        """
        Run complete performance test comparing both methods.
        
        Args:
            frame_count: Number of frames to test with
            
        Returns:
            Complete performance comparison
        """
        self.logger.info(f"Starting performance test with {frame_count} frames...")
        
        # Generate test frames
        self.generate_test_frames(frame_count)
        
        results = {}
        
        try:
            # Test base64 storage
            self.logger.info("=" * 50)
            self.logger.info("Testing Base64 Storage")
            self.logger.info("=" * 50)
            results["base64"] = self.test_base64_storage()
            
            # Test file storage
            self.logger.info("=" * 50)
            self.logger.info("Testing File Storage")
            self.logger.info("=" * 50)
            results["file_storage"] = self.test_file_storage()
            
            # Test file storage with original
            self.logger.info("=" * 50)
            self.logger.info("Testing File Storage with Original")
            self.logger.info("=" * 50)
            results["file_storage_with_original"] = self.test_file_storage_with_original()
            
            # Calculate improvements
            base64_save_rate = results["base64"]["save_rate"]
            file_save_rate = results["file_storage"]["save_rate"]
            file_original_save_rate = results["file_storage_with_original"]["save_rate"]
            
            base64_retrieval_rate = results["base64"]["retrieval_rate"]
            file_retrieval_rate = results["file_storage"]["retrieval_rate"]
            file_original_retrieval_rate = results["file_storage_with_original"]["retrieval_rate"]
            
            results["improvements"] = {
                "save_speedup": file_save_rate / base64_save_rate if base64_save_rate > 0 else 0,
                "retrieval_speedup": file_retrieval_rate / base64_retrieval_rate if base64_retrieval_rate > 0 else 0,
                "save_speedup_with_original": file_original_save_rate / base64_save_rate if base64_save_rate > 0 else 0,
                "retrieval_speedup_with_original": file_original_retrieval_rate / base64_retrieval_rate if base64_retrieval_rate > 0 else 0
            }
            
        finally:
            # Clean up test data
            if "base64" in results:
                self.cleanup_test_data(results["base64"]["uuids"], "base64")
            if "file_storage" in results:
                self.cleanup_test_data(results["file_storage"]["uuids"], "file_storage")
            if "file_storage_with_original" in results:
                self.cleanup_test_data(results["file_storage_with_original"]["uuids"], "file_storage")
        
        return results
    
    def print_results(self, results: Dict[str, Any]):
        """Print performance test results in a readable format."""
        print("\n" + "=" * 80)
        print("PERFORMANCE TEST RESULTS")
        print("=" * 80)
        
        for method, data in results.items():
            if method == "improvements":
                continue
                
            print(f"\n{method.upper().replace('_', ' ')}:")
            print(f"  Frames saved: {data['frames_saved']}")
            print(f"  Save time: {data['save_time']:.2f} seconds")
            print(f"  Save rate: {data['save_rate']:.2f} frames/second")
            print(f"  Retrieval time: {data['retrieval_time']:.2f} seconds")
            print(f"  Retrieval rate: {data['retrieval_rate']:.2f} frames/second")
            if 'metadata_time' in data:
                print(f"  Metadata time: {data['metadata_time']:.2f} seconds")
                print(f"  Metadata rate: {data['metadata_rate']:.2f} frames/second")
        
        if "improvements" in results:
            print(f"\nIMPROVEMENTS (File Storage vs Base64):")
            print(f"  Save speedup: {results['improvements']['save_speedup']:.2f}x")
            print(f"  Retrieval speedup: {results['improvements']['retrieval_speedup']:.2f}x")
            print(f"  Save speedup (with original): {results['improvements']['save_speedup_with_original']:.2f}x")
            print(f"  Retrieval speedup (with original): {results['improvements']['retrieval_speedup_with_original']:.2f}x")


def main():
    """Main performance test function."""
    import argparse
    
    parser = argparse.ArgumentParser(description="Performance test: Base64 vs File Storage")
    parser.add_argument("--frames", type=int, default=100,
                       help="Number of frames to test with (default: 100)")
    parser.add_argument("--verbose", action="store_true",
                       help="Enable verbose logging")
    
    args = parser.parse_args()
    
    # Setup logging
    log_level = logging.DEBUG if args.verbose else logging.INFO
    logging.basicConfig(
        level=log_level,
        format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
    )
    
    try:
        # Run performance test
        tester = PerformanceTester()
        results = tester.run_performance_test(args.frames)
        
        # Print results
        tester.print_results(results)
        
        print(f"\n✅ Performance test completed with {args.frames} frames")
        
    except Exception as e:
        print(f"❌ Performance test failed: {e}")
        sys.exit(1)


if __name__ == "__main__":
    main()
