"""
Migration Script: MongoDB Base64 to File Storage

This script migrates existing frames from base64 storage in MongoDB
to the new efficient file-based storage system.
"""

import os
import sys
import base64
import logging
from pathlib import Path
from typing import Dict, Any, Optional
import argparse
from datetime import datetime

# Add the src directory to the Python path
sys.path.insert(0, os.path.abspath(os.path.join(os.path.dirname(__file__), '../..')))

from mongodb.database_manager import DatabaseManager
from mongodb.frame_database import FrameDatabase  # Old version
from mongodb.frame_database_v2 import FrameDatabaseV2  # New version
from mongodb.file_storage_manager import FileStorageManager


class MigrationManager:
    """Manages migration from base64 storage to file storage."""
    
    def __init__(self, storage_path: str = "data/frames"):
        """
        Initialize the migration manager.
        
        Args:
            storage_path: Base path for new file storage
        """
        self.storage_path = storage_path
        self.logger = logging.getLogger(__name__)
        
        # Initialize database connections
        self.db_manager = DatabaseManager()
        if not self.db_manager.connect():
            raise RuntimeError("Failed to connect to MongoDB")
        
        # Initialize old and new frame databases
        self.old_frame_db = FrameDatabase(self.db_manager)
        self.new_frame_db = FrameDatabaseV2(self.db_manager, storage_path)
        self.file_storage = FileStorageManager(storage_path)
        
    def migrate_frame(self, frame_doc: Dict[str, Any]) -> bool:
        """
        Migrate a single frame from base64 to file storage.
        
        Args:
            frame_doc: MongoDB document with base64 frame data
            
        Returns:
            True if successful, False otherwise
        """
        try:
            frame_uuid = frame_doc["_id"]
            self.logger.info(f"Migrating frame: {frame_uuid}")
            
            # Decode base64 frame data
            frame_data = None
            original_frame_data = None
            
            if "frame_data" in frame_doc and frame_doc["frame_data"]:
                frame_data = base64.b64decode(frame_doc["frame_data"])
            
            if "original_frame_data" in frame_doc and frame_doc["original_frame_data"]:
                original_frame_data = base64.b64decode(frame_doc["original_frame_data"])
            
            # Convert to OpenCV frames
            processed_frame = None
            original_frame = None
            
            if frame_data:
                import cv2
                import numpy as np
                nparr = np.frombuffer(frame_data, np.uint8)
                processed_frame = cv2.imdecode(nparr, cv2.IMREAD_COLOR)
            
            if original_frame_data:
                import cv2
                import numpy as np
                nparr = np.frombuffer(original_frame_data, np.uint8)
                original_frame = cv2.imdecode(nparr, cv2.IMREAD_COLOR)
            
            # Save to new file-based system
            if original_frame is not None and processed_frame is not None:
                # Both frames available
                success = self.new_frame_db.save_frame_with_original(
                    original_frame, processed_frame, frame_doc.get("metadata", {})
                )
            elif processed_frame is not None:
                # Only processed frame available
                success = self.new_frame_db.save_frame(
                    processed_frame, frame_doc.get("metadata", {})
                )
            else:
                self.logger.warning(f"No valid frame data found for: {frame_uuid}")
                return False
            
            if success:
                self.logger.info(f"Successfully migrated frame: {frame_uuid}")
                return True
            else:
                self.logger.error(f"Failed to save migrated frame: {frame_uuid}")
                return False
                
        except Exception as e:
            self.logger.error(f"Error migrating frame {frame_doc.get('_id', 'unknown')}: {e}")
            return False
    
    def migrate_all_frames(self, batch_size: int = 100, dry_run: bool = False) -> Dict[str, int]:
        """
        Migrate all frames from base64 storage to file storage.
        
        Args:
            batch_size: Number of frames to process in each batch
            dry_run: If True, only count frames without migrating
            
        Returns:
            Dictionary with migration statistics
        """
        try:
            collection = self.db_manager.get_collection("captured_frames")
            if collection is None:
                raise RuntimeError("Failed to get captured_frames collection")
            
            # Get total count
            total_frames = collection.count_documents({})
            self.logger.info(f"Found {total_frames} frames to migrate")
            
            if dry_run:
                return {
                    "total_frames": total_frames,
                    "migrated": 0,
                    "failed": 0,
                    "skipped": 0
                }
            
            # Migration statistics
            stats = {
                "total_frames": total_frames,
                "migrated": 0,
                "failed": 0,
                "skipped": 0
            }
            
            # Process frames in batches
            skip = 0
            while skip < total_frames:
                self.logger.info(f"Processing batch: {skip + 1} to {min(skip + batch_size, total_frames)}")
                
                # Get batch of frames
                cursor = collection.find({}).skip(skip).limit(batch_size)
                
                for frame_doc in cursor:
                    try:
                        # Check if frame already migrated (has file paths)
                        if "original_image_path" in frame_doc or "processed_image_path" in frame_doc:
                            self.logger.debug(f"Frame already migrated: {frame_doc['_id']}")
                            stats["skipped"] += 1
                            continue
                        
                        # Migrate frame
                        if self.migrate_frame(frame_doc):
                            stats["migrated"] += 1
                        else:
                            stats["failed"] += 1
                            
                    except Exception as e:
                        self.logger.error(f"Error processing frame {frame_doc.get('_id', 'unknown')}: {e}")
                        stats["failed"] += 1
                
                skip += batch_size
                
                # Log progress
                progress = (skip / total_frames) * 100
                self.logger.info(f"Migration progress: {progress:.1f}% ({stats['migrated']} migrated, {stats['failed']} failed)")
            
            return stats
            
        except Exception as e:
            self.logger.error(f"Error during migration: {e}")
            return {"error": str(e)}
    
    def verify_migration(self, sample_size: int = 10) -> Dict[str, Any]:
        """
        Verify that migration was successful by checking sample frames.
        
        Args:
            sample_size: Number of frames to verify
            
        Returns:
            Verification results
        """
        try:
            collection = self.db_manager.get_collection("captured_frames")
            if collection is None:
                raise RuntimeError("Failed to get captured_frames collection")
            
            # Get sample frames
            sample_frames = list(collection.find({}).limit(sample_size))
            
            verification_results = {
                "total_checked": len(sample_frames),
                "file_exists": 0,
                "file_missing": 0,
                "load_successful": 0,
                "load_failed": 0,
                "errors": []
            }
            
            for frame_doc in sample_frames:
                frame_uuid = frame_doc["_id"]
                
                try:
                    # Check if files exist
                    has_original = "original_image_path" in frame_doc
                    has_processed = "processed_image_path" in frame_doc
                    
                    if has_original or has_processed:
                        verification_results["file_exists"] += 1
                        
                        # Try to load frame
                        frame = self.new_frame_db.get_frame(frame_uuid, "processed")
                        if frame is not None:
                            verification_results["load_successful"] += 1
                        else:
                            verification_results["load_failed"] += 1
                            verification_results["errors"].append(f"Failed to load frame: {frame_uuid}")
                    else:
                        verification_results["file_missing"] += 1
                        verification_results["errors"].append(f"No file paths for frame: {frame_uuid}")
                        
                except Exception as e:
                    verification_results["load_failed"] += 1
                    verification_results["errors"].append(f"Error checking frame {frame_uuid}: {e}")
            
            return verification_results
            
        except Exception as e:
            self.logger.error(f"Error during verification: {e}")
            return {"error": str(e)}
    
    def cleanup_old_data(self, keep_backup: bool = True) -> bool:
        """
        Clean up old base64 data from MongoDB after successful migration.
        
        Args:
            keep_backup: If True, rename fields instead of deleting them
            
        Returns:
            True if successful, False otherwise
        """
        try:
            collection = self.db_manager.get_collection("captured_frames")
            if collection is None:
                return False
            
            if keep_backup:
                # Rename old fields to backup fields
                result = collection.update_many(
                    {"frame_data": {"$exists": True}},
                    {"$rename": {
                        "frame_data": "frame_data_backup",
                        "original_frame_data": "original_frame_data_backup"
                    }}
                )
                self.logger.info(f"Renamed {result.modified_count} documents to backup fields")
            else:
                # Remove old fields completely
                result = collection.update_many(
                    {},
                    {"$unset": {
                        "frame_data": "",
                        "original_frame_data": ""
                    }}
                )
                self.logger.info(f"Removed base64 data from {result.modified_count} documents")
            
            return True
            
        except Exception as e:
            self.logger.error(f"Error cleaning up old data: {e}")
            return False


def main():
    """Main migration function."""
    parser = argparse.ArgumentParser(description="Migrate MongoDB frames from base64 to file storage")
    parser.add_argument("--storage-path", default="data/frames",
                       help="Base path for file storage (default: data/frames)")
    parser.add_argument("--batch-size", type=int, default=100,
                       help="Batch size for processing (default: 100)")
    parser.add_argument("--dry-run", action="store_true",
                       help="Count frames without migrating")
    parser.add_argument("--verify", action="store_true",
                       help="Verify migration results")
    parser.add_argument("--cleanup", action="store_true",
                       help="Clean up old base64 data after migration")
    parser.add_argument("--keep-backup", action="store_true",
                       help="Keep backup of old data when cleaning up")
    
    args = parser.parse_args()
    
    # Setup logging
    logging.basicConfig(
        level=logging.INFO,
        format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
    )
    
    try:
        # Initialize migration manager
        migration = MigrationManager(args.storage_path)
        
        if args.verify:
            # Verify migration
            print("🔍 Verifying migration...")
            results = migration.verify_migration()
            print(f"✅ Verification complete:")
            print(f"   Total checked: {results.get('total_checked', 0)}")
            print(f"   Files exist: {results.get('file_exists', 0)}")
            print(f"   Files missing: {results.get('file_missing', 0)}")
            print(f"   Load successful: {results.get('load_successful', 0)}")
            print(f"   Load failed: {results.get('load_failed', 0)}")
            
            if results.get('errors'):
                print(f"   Errors: {len(results['errors'])}")
                for error in results['errors'][:5]:  # Show first 5 errors
                    print(f"     - {error}")
        
        elif args.cleanup:
            # Clean up old data
            print("🧹 Cleaning up old base64 data...")
            success = migration.cleanup_old_data(args.keep_backup)
            if success:
                print("✅ Cleanup completed successfully")
            else:
                print("❌ Cleanup failed")
        
        else:
            # Run migration
            print("🚀 Starting migration from base64 to file storage...")
            print(f"📁 Storage path: {args.storage_path}")
            print(f"📦 Batch size: {args.batch_size}")
            
            if args.dry_run:
                print("🔍 DRY RUN - No actual migration will be performed")
            
            stats = migration.migrate_all_frames(args.batch_size, args.dry_run)
            
            print("\n📊 Migration Results:")
            print(f"   Total frames: {stats.get('total_frames', 0)}")
            print(f"   Migrated: {stats.get('migrated', 0)}")
            print(f"   Failed: {stats.get('failed', 0)}")
            print(f"   Skipped: {stats.get('skipped', 0)}")
            
            if stats.get('migrated', 0) > 0:
                success_rate = (stats['migrated'] / stats['total_frames']) * 100
                print(f"   Success rate: {success_rate:.1f}%")
            
            if not args.dry_run and stats.get('migrated', 0) > 0:
                print("\n💡 Next steps:")
                print("   1. Run with --verify to check migration results")
                print("   2. Run with --cleanup to remove old base64 data")
                print("   3. Update your application to use FrameDatabaseV2")
    
    except Exception as e:
        print(f"❌ Migration failed: {e}")
        sys.exit(1)


if __name__ == "__main__":
    main()
