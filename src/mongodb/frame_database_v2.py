"""
Frame Database V2 for Birds of Play

Efficient frame storage using local file system with MongoDB metadata only.
This replaces the inefficient base64 storage approach.
"""

import uuid
import logging
from typing import Optional, Dict, Any, List
from datetime import datetime
import cv2
import numpy as np
from pymongo.collection import Collection

from .database_manager import DatabaseManager
from .file_storage_manager import FileStorageManager


class FrameDatabaseV2:
    """Manages frame storage using local files with MongoDB metadata."""
    
    def __init__(self, db_manager: DatabaseManager, storage_path: str = "data/frames"):
        """
        Initialize the frame database.
        
        Args:
            db_manager: Database manager instance
            storage_path: Base path for local file storage
        """
        self.db_manager = db_manager
        self.collection_name = "captured_frames"
        self.logger = logging.getLogger(__name__)
        
        # Initialize file storage manager
        self.file_storage = FileStorageManager(storage_path)
        
        # Create indexes
        self._create_indexes()
        
    def _create_indexes(self):
        """Create database indexes for efficient queries."""
        try:
            collection = self.db_manager.get_collection(self.collection_name)
            if collection is None:
                return
            
            # Create indexes
            collection.create_index("timestamp")
            collection.create_index("created_at")
            collection.create_index("metadata.source")
            collection.create_index("metadata.motion_detected")
            collection.create_index("metadata.motion_regions")
            
            self.logger.info("Database indexes created successfully")
            
        except Exception as e:
            self.logger.warning(f"Could not create indexes: {e}")
    
    def save_frame(self, frame: np.ndarray, metadata: Optional[Dict[str, Any]] = None) -> Optional[str]:
        """
        Save a frame to local storage with metadata in MongoDB, with automatic thumbnail generation.
        
        Args:
            frame: OpenCV frame (numpy array)
            metadata: Additional metadata to store with the frame
            
        Returns:
            UUID of the saved frame or None if failed
        """
        try:
            collection = self.db_manager.get_collection(self.collection_name)
            if collection is None:
                return None
            
            # Generate unique UUID
            frame_uuid = str(uuid.uuid4())
            
            # Save frame to local storage
            file_path = self.file_storage.save_frame(frame, frame_uuid, "processed")
            if not file_path:
                return None
            
            # Generate thumbnail
            thumbnail_path = self.file_storage.save_thumbnail(frame, frame_uuid, "processed")
            
            # Prepare document (no image data, just metadata and file paths)
            document = {
                "_id": frame_uuid,
                "processed_image_path": file_path,
                "processed_thumbnail_path": thumbnail_path,
                "frame_shape": frame.shape,
                "frame_dtype": str(frame.dtype),
                "timestamp": datetime.utcnow(),
                "created_at": datetime.utcnow(),
                "metadata": metadata or {}
            }
            
            # Insert into database
            result = collection.insert_one(document)
            
            if result.inserted_id:
                self.logger.info(f"Saved frame with UUID: {frame_uuid}")
                return frame_uuid
            else:
                # Clean up files if database insert failed
                self.file_storage.delete_frame(frame_uuid, "processed")
                if thumbnail_path:
                    self.file_storage.delete_thumbnail(frame_uuid, "processed")
                return None
                
        except Exception as e:
            self.logger.error(f"Failed to save frame: {e}")
            return None
    
    def save_frame_with_original(self, original_frame: np.ndarray, processed_frame: np.ndarray, 
                                metadata: Optional[Dict[str, Any]] = None) -> Optional[str]:
        """
        Save both original and processed frames with metadata in MongoDB.
        
        Args:
            original_frame: Original clean frame
            processed_frame: Processed frame with overlays/annotations
            metadata: Additional metadata to store with the frame
            
        Returns:
            UUID of the saved frame or None if failed
        """
        try:
            collection = self.db_manager.get_collection(self.collection_name)
            if collection is None:
                return None
            
            # Generate unique UUID
            frame_uuid = str(uuid.uuid4())
            
            # Save both frames to local storage with thumbnails
            original_path, processed_path, original_thumbnail_path, processed_thumbnail_path = self.file_storage.save_both_frames(
                original_frame, processed_frame, frame_uuid, create_thumbnails=True
            )
            
            if not original_path or not processed_path:
                return None
            
            # Prepare document (no image data, just metadata and file paths)
            document = {
                "_id": frame_uuid,
                "original_image_path": original_path,
                "processed_image_path": processed_path,
                "original_thumbnail_path": original_thumbnail_path,
                "processed_thumbnail_path": processed_thumbnail_path,
                "frame_shape": processed_frame.shape,
                "original_frame_shape": original_frame.shape,
                "frame_dtype": str(processed_frame.dtype),
                "original_frame_dtype": str(original_frame.dtype),
                "timestamp": datetime.utcnow(),
                "created_at": datetime.utcnow(),
                "metadata": metadata or {}
            }
            
            # Insert into database
            result = collection.insert_one(document)
            
            if result.inserted_id:
                self.logger.info(f"Saved both frames with UUID: {frame_uuid}")
                return frame_uuid
            else:
                # Clean up files if database insert failed
                self.file_storage.delete_frame(frame_uuid, "both")
                return None
                
        except Exception as e:
            self.logger.error(f"Failed to save frame with original: {e}")
            return None
    
    def get_frame(self, frame_uuid: str, image_type: str = "processed") -> Optional[np.ndarray]:
        """
        Retrieve a frame from local storage.
        
        Args:
            frame_uuid: UUID of the frame to retrieve
            image_type: Type of image ("original" or "processed")
            
        Returns:
            OpenCV frame (numpy array) or None if not found
        """
        try:
            # Check if frame exists in database
            collection = self.db_manager.get_collection(self.collection_name)
            if collection is None:
                return None
            
            frame_doc = collection.find_one({"_id": frame_uuid})
            if not frame_doc:
                self.logger.warning(f"Frame not found in database: {frame_uuid}")
                return None
            
            # Load frame from local storage
            frame = self.file_storage.load_frame(frame_uuid, image_type)
            if frame is not None:
                self.logger.debug(f"Retrieved {image_type} frame: {frame_uuid}")
            else:
                self.logger.warning(f"Frame file not found: {frame_uuid} ({image_type})")
            
            return frame
            
        except Exception as e:
            self.logger.error(f"Failed to retrieve frame {frame_uuid}: {e}")
            return None
    
    def get_frame_metadata(self, frame_uuid: str) -> Optional[Dict[str, Any]]:
        """
        Get frame metadata without loading the image data.
        
        Args:
            frame_uuid: UUID of the frame
            
        Returns:
            Frame metadata dictionary or None if not found
        """
        try:
            collection = self.db_manager.get_collection(self.collection_name)
            if collection is None:
                return None
            
            # Project to exclude image data (not needed since we don't store it)
            frame_doc = collection.find_one(
                {"_id": frame_uuid},
                {"_id": 1, "frame_shape": 1, "original_frame_shape": 1, 
                 "frame_dtype": 1, "original_frame_dtype": 1, 
                 "timestamp": 1, "created_at": 1, "metadata": 1,
                 "original_image_path": 1, "processed_image_path": 1}
            )
            
            if frame_doc:
                self.logger.debug(f"Retrieved metadata for frame: {frame_uuid}")
                return frame_doc
            else:
                self.logger.warning(f"Frame metadata not found: {frame_uuid}")
                return None
                
        except Exception as e:
            self.logger.error(f"Failed to retrieve frame metadata {frame_uuid}: {e}")
            return None
    
    def list_frames(self, limit: int = 100, skip: int = 0, 
                   source: Optional[str] = None) -> List[Dict[str, Any]]:
        """
        List frames with metadata (no image data).
        
        Args:
            limit: Maximum number of frames to return
            skip: Number of frames to skip
            source: Filter by source (e.g., "webcam", "video")
            
        Returns:
            List of frame metadata dictionaries
        """
        try:
            collection = self.db_manager.get_collection(self.collection_name)
            if collection is None:
                return []
            
            # Build query
            query = {}
            if source:
                query["metadata.source"] = source
            
            # Get frames with metadata only (no image data)
            cursor = collection.find(
                query,
                {"_id": 1, "frame_shape": 1, "original_frame_shape": 1,
                 "timestamp": 1, "created_at": 1, "metadata": 1,
                 "original_image_path": 1, "processed_image_path": 1}
            ).sort("timestamp", -1).skip(skip).limit(limit)
            
            frames = list(cursor)
            self.logger.debug(f"Listed {len(frames)} frames (limit={limit}, skip={skip})")
            
            return frames
            
        except Exception as e:
            self.logger.error(f"Failed to list frames: {e}")
            return []
    
    def delete_frame(self, frame_uuid: str) -> bool:
        """
        Delete a frame and its associated files.
        
        Args:
            frame_uuid: UUID of the frame to delete
            
        Returns:
            True if successful, False otherwise
        """
        try:
            collection = self.db_manager.get_collection(self.collection_name)
            if collection is None:
                return False
            
            # Delete from database
            result = collection.delete_one({"_id": frame_uuid})
            
            if result.deleted_count > 0:
                # Delete associated files
                self.file_storage.delete_frame(frame_uuid, "both")
                self.logger.info(f"Deleted frame: {frame_uuid}")
                return True
            else:
                self.logger.warning(f"Frame not found for deletion: {frame_uuid}")
                return False
                
        except Exception as e:
            self.logger.error(f"Failed to delete frame {frame_uuid}: {e}")
            return False
    
    def get_frame_count(self) -> int:
        """
        Get the total number of frames in the database.
        
        Returns:
            Number of frames
        """
        try:
            collection = self.db_manager.get_collection(self.collection_name)
            if collection is None:
                return 0
            
            count = collection.count_documents({})
            return count
            
        except Exception as e:
            self.logger.error(f"Failed to get frame count: {e}")
            return 0
    
    def get_storage_stats(self) -> Dict[str, Any]:
        """
        Get comprehensive storage statistics.
        
        Returns:
            Dictionary with storage statistics
        """
        try:
            # Get file storage stats
            file_stats = self.file_storage.get_storage_stats()
            
            # Get database stats
            db_stats = {
                "total_frames": self.get_frame_count(),
                "frames_with_original": 0,
                "frames_with_processed": 0
            }
            
            # Count frames with different image types
            collection = self.db_manager.get_collection(self.collection_name)
            if collection:
                db_stats["frames_with_original"] = collection.count_documents(
                    {"original_image_path": {"$exists": True}}
                )
                db_stats["frames_with_processed"] = collection.count_documents(
                    {"processed_image_path": {"$exists": True}}
                )
            
            # Combine stats
            stats = {
                **file_stats,
                **db_stats,
                "storage_efficiency": "file_based"  # Indicate this is the efficient version
            }
            
            return stats
            
        except Exception as e:
            self.logger.error(f"Failed to get storage stats: {e}")
            return {}
    
    def cleanup_old_frames(self, max_age_days: int = 30) -> int:
        """
        Clean up old frames from both database and file system.
        
        Args:
            max_age_days: Maximum age in days for frames to keep
            
        Returns:
            Number of frames deleted
        """
        try:
            collection = self.db_manager.get_collection(self.collection_name)
            if collection is None:
                return 0
            
            # Calculate cutoff date
            cutoff_date = datetime.utcnow().replace(hour=0, minute=0, second=0, microsecond=0)
            cutoff_date = cutoff_date.replace(day=cutoff_date.day - max_age_days)
            
            # Find old frames
            old_frames = collection.find(
                {"timestamp": {"$lt": cutoff_date}},
                {"_id": 1}
            )
            
            deleted_count = 0
            for frame_doc in old_frames:
                frame_uuid = frame_doc["_id"]
                if self.delete_frame(frame_uuid):
                    deleted_count += 1
            
            if deleted_count > 0:
                self.logger.info(f"Cleaned up {deleted_count} old frames")
            
            return deleted_count
            
        except Exception as e:
            self.logger.error(f"Failed to cleanup old frames: {e}")
            return 0
    
    def cleanup_after_processing(self, frame_uuid: str, keep_thumbnails: bool = True) -> bool:
        """
        Clean up full-resolution images after they've been processed by the image detector.
        This helps save disk space while keeping thumbnails for quick preview.
        
        Args:
            frame_uuid: UUID of the frame to clean up
            keep_thumbnails: Whether to keep thumbnail versions
            
        Returns:
            True if successful, False otherwise
        """
        try:
            # Update MongoDB document to reflect that full-resolution images are deleted
            collection = self.db_manager.get_collection(self.collection_name)
            if collection is None:
                return False
            
            # Remove full-resolution image paths from document
            update_doc = {
                "$unset": {
                    "original_image_path": "",
                    "processed_image_path": ""
                }
            }
            
            # Optionally remove thumbnail paths too
            if not keep_thumbnails:
                update_doc["$unset"]["original_thumbnail_path"] = ""
                update_doc["$unset"]["processed_thumbnail_path"] = ""
            
            # Add processing status
            update_doc["$set"] = {
                "processing_status": "completed",
                "full_resolution_cleaned": True,
                "cleanup_timestamp": datetime.utcnow()
            }
            
            result = collection.update_one({"_id": frame_uuid}, update_doc)
            
            if result.modified_count > 0:
                # Delete actual files
                self.file_storage.delete_full_resolution_images(frame_uuid, keep_thumbnails)
                self.logger.info(f"Cleaned up frame after processing: {frame_uuid}")
                return True
            else:
                self.logger.warning(f"Frame not found for cleanup: {frame_uuid}")
                return False
                
        except Exception as e:
            self.logger.error(f"Error cleaning up frame after processing {frame_uuid}: {e}")
            return False
