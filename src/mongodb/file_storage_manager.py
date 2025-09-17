"""
File Storage Manager for Birds of Play

Handles efficient local file storage of images with UUID-based organization.
This replaces the inefficient base64 storage in MongoDB documents.
"""

import os
import uuid
import shutil
import logging
from pathlib import Path
from typing import Optional, Dict, Any, Tuple
import cv2
import numpy as np
from datetime import datetime

class FileStorageManager:
    """Manages local file storage for frame images with UUID-based organization."""
    
    def __init__(self, base_storage_path: str = "data/frames"):
        """
        Initialize the file storage manager.
        
        Args:
            base_storage_path: Base directory for storing frame images
        """
        self.base_path = Path(base_storage_path)
        self.original_path = self.base_path / "original"
        self.processed_path = self.base_path / "processed"
        self.original_thumbnails_path = self.base_path / "original_thumbnails"
        self.processed_thumbnails_path = self.base_path / "processed_thumbnails"
        self.logger = logging.getLogger(__name__)
        
        # Create directory structure
        self._ensure_directories()
        
    def _ensure_directories(self):
        """Ensure all required directories exist."""
        try:
            self.original_path.mkdir(parents=True, exist_ok=True)
            self.processed_path.mkdir(parents=True, exist_ok=True)
            self.original_thumbnails_path.mkdir(parents=True, exist_ok=True)
            self.processed_thumbnails_path.mkdir(parents=True, exist_ok=True)
            self.logger.info(f"File storage directories ready: {self.base_path}")
        except Exception as e:
            self.logger.error(f"Failed to create storage directories: {e}")
            raise
    
    def save_frame(self, frame: np.ndarray, frame_uuid: str, 
                   image_type: str = "processed", quality: int = 95) -> Optional[str]:
        """
        Save a frame image to local storage.
        
        Args:
            frame: OpenCV frame (numpy array)
            frame_uuid: Unique identifier for the frame
            image_type: Type of image ("original" or "processed")
            quality: JPEG quality (1-100)
            
        Returns:
            Path to saved image file or None if failed
        """
        try:
            # Determine storage path
            if image_type == "original":
                storage_path = self.original_path
            elif image_type == "processed":
                storage_path = self.processed_path
            else:
                raise ValueError(f"Invalid image_type: {image_type}")
            
            # Create filename with UUID
            filename = f"{frame_uuid}.jpg"
            file_path = storage_path / filename
            
            # Save image with specified quality
            success = cv2.imwrite(str(file_path), frame, 
                                [cv2.IMWRITE_JPEG_QUALITY, quality])
            
            if success:
                self.logger.debug(f"Saved {image_type} frame: {file_path}")
                return str(file_path)
            else:
                self.logger.error(f"Failed to save {image_type} frame: {file_path}")
                return None
                
        except Exception as e:
            self.logger.error(f"Error saving {image_type} frame {frame_uuid}: {e}")
            return None
    
    def save_thumbnail(self, frame: np.ndarray, frame_uuid: str, 
                      image_type: str = "processed", thumbnail_size: Tuple[int, int] = (320, 240)) -> Optional[str]:
        """
        Save a thumbnail version of a frame.
        
        Args:
            frame: OpenCV frame (numpy array)
            frame_uuid: Unique identifier for the frame
            image_type: Type of image ("original" or "processed")
            thumbnail_size: Size of thumbnail (width, height)
            
        Returns:
            Path to saved thumbnail file or None if failed
        """
        try:
            # Determine storage path
            if image_type == "original":
                storage_path = self.original_thumbnails_path
            elif image_type == "processed":
                storage_path = self.processed_thumbnails_path
            else:
                raise ValueError(f"Invalid image_type: {image_type}")
            
            # Create filename with UUID
            filename = f"{frame_uuid}.jpg"
            file_path = storage_path / filename
            
            # Resize frame to thumbnail size
            thumbnail = cv2.resize(frame, thumbnail_size, interpolation=cv2.INTER_AREA)
            
            # Save thumbnail with lower quality for smaller file size
            success = cv2.imwrite(str(file_path), thumbnail, 
                                [cv2.IMWRITE_JPEG_QUALITY, 75])
            
            if success:
                self.logger.debug(f"Saved {image_type} thumbnail: {file_path}")
                return str(file_path)
            else:
                self.logger.error(f"Failed to save {image_type} thumbnail: {file_path}")
                return None
                
        except Exception as e:
            self.logger.error(f"Error saving {image_type} thumbnail {frame_uuid}: {e}")
            return None
    
    def load_thumbnail(self, frame_uuid: str, image_type: str = "processed") -> Optional[np.ndarray]:
        """
        Load a thumbnail image from local storage.
        
        Args:
            frame_uuid: Unique identifier for the frame
            image_type: Type of image ("original" or "processed")
            
        Returns:
            OpenCV frame (numpy array) or None if not found
        """
        try:
            # Determine storage path
            if image_type == "original":
                storage_path = self.original_thumbnails_path
            elif image_type == "processed":
                storage_path = self.processed_thumbnails_path
            else:
                raise ValueError(f"Invalid image_type: {image_type}")
            
            # Create filename with UUID
            filename = f"{frame_uuid}.jpg"
            file_path = storage_path / filename
            
            if not file_path.exists():
                self.logger.warning(f"Thumbnail not found: {file_path}")
                return None
            
            # Load thumbnail
            thumbnail = cv2.imread(str(file_path))
            if thumbnail is not None:
                self.logger.debug(f"Loaded {image_type} thumbnail: {file_path}")
                return thumbnail
            else:
                self.logger.error(f"Failed to load {image_type} thumbnail: {file_path}")
                return None
                
        except Exception as e:
            self.logger.error(f"Error loading {image_type} thumbnail {frame_uuid}: {e}")
            return None
    
    def delete_thumbnail(self, frame_uuid: str, image_type: str = "both") -> bool:
        """
        Delete thumbnail images from local storage.
        
        Args:
            frame_uuid: Unique identifier for the frame
            image_type: Type of image ("original", "processed", or "both")
            
        Returns:
            True if successful, False otherwise
        """
        try:
            success = True
            
            if image_type in ["original", "both"]:
                original_path = self.original_thumbnails_path / f"{frame_uuid}.jpg"
                if original_path.exists():
                    original_path.unlink()
                    self.logger.debug(f"Deleted original thumbnail: {original_path}")
                else:
                    self.logger.warning(f"Original thumbnail not found: {original_path}")
            
            if image_type in ["processed", "both"]:
                processed_path = self.processed_thumbnails_path / f"{frame_uuid}.jpg"
                if processed_path.exists():
                    processed_path.unlink()
                    self.logger.debug(f"Deleted processed thumbnail: {processed_path}")
                else:
                    self.logger.warning(f"Processed thumbnail not found: {processed_path}")
            
            return success
            
        except Exception as e:
            self.logger.error(f"Error deleting thumbnail {frame_uuid}: {e}")
            return False
    
    def save_both_frames(self, original_frame: np.ndarray, processed_frame: np.ndarray,
                        frame_uuid: str, quality: int = 95, create_thumbnails: bool = True) -> Tuple[Optional[str], Optional[str], Optional[str], Optional[str]]:
        """
        Save both original and processed frames with optional thumbnails.
        
        Args:
            original_frame: Original clean frame
            processed_frame: Processed frame with overlays
            frame_uuid: Unique identifier for the frame
            quality: JPEG quality (1-100)
            create_thumbnails: Whether to create thumbnail versions
            
        Returns:
            Tuple of (original_path, processed_path, original_thumbnail_path, processed_thumbnail_path) or (None, None, None, None) if failed
        """
        try:
            original_path = self.save_frame(original_frame, frame_uuid, "original", quality)
            processed_path = self.save_frame(processed_frame, frame_uuid, "processed", quality)
            
            original_thumbnail_path = None
            processed_thumbnail_path = None
            
            if create_thumbnails:
                original_thumbnail_path = self.save_thumbnail(original_frame, frame_uuid, "original")
                processed_thumbnail_path = self.save_thumbnail(processed_frame, frame_uuid, "processed")
            
            if original_path and processed_path:
                self.logger.info(f"Saved both frames for UUID: {frame_uuid}")
                return original_path, processed_path, original_thumbnail_path, processed_thumbnail_path
            else:
                # Clean up partial saves
                if original_path:
                    self.delete_frame(frame_uuid, "original")
                if processed_path:
                    self.delete_frame(frame_uuid, "processed")
                if original_thumbnail_path:
                    self.delete_thumbnail(frame_uuid, "original")
                if processed_thumbnail_path:
                    self.delete_thumbnail(frame_uuid, "processed")
                return None, None, None, None
                
        except Exception as e:
            self.logger.error(f"Error saving both frames for UUID {frame_uuid}: {e}")
            return None, None, None, None
    
    def load_frame(self, frame_uuid: str, image_type: str = "processed") -> Optional[np.ndarray]:
        """
        Load a frame image from local storage.
        
        Args:
            frame_uuid: Unique identifier for the frame
            image_type: Type of image ("original" or "processed")
            
        Returns:
            OpenCV frame (numpy array) or None if not found
        """
        try:
            # Determine storage path
            if image_type == "original":
                storage_path = self.original_path
            elif image_type == "processed":
                storage_path = self.processed_path
            else:
                raise ValueError(f"Invalid image_type: {image_type}")
            
            # Create filename with UUID
            filename = f"{frame_uuid}.jpg"
            file_path = storage_path / filename
            
            if not file_path.exists():
                self.logger.warning(f"Frame not found: {file_path}")
                return None
            
            # Load image
            frame = cv2.imread(str(file_path))
            if frame is not None:
                self.logger.debug(f"Loaded {image_type} frame: {file_path}")
                return frame
            else:
                self.logger.error(f"Failed to load {image_type} frame: {file_path}")
                return None
                
        except Exception as e:
            self.logger.error(f"Error loading {image_type} frame {frame_uuid}: {e}")
            return None
    
    def delete_frame(self, frame_uuid: str, image_type: str = "both") -> bool:
        """
        Delete frame images from local storage.
        
        Args:
            frame_uuid: Unique identifier for the frame
            image_type: Type of image ("original", "processed", or "both")
            
        Returns:
            True if successful, False otherwise
        """
        try:
            success = True
            
            if image_type in ["original", "both"]:
                original_path = self.original_path / f"{frame_uuid}.jpg"
                if original_path.exists():
                    original_path.unlink()
                    self.logger.debug(f"Deleted original frame: {original_path}")
                else:
                    self.logger.warning(f"Original frame not found: {original_path}")
            
            if image_type in ["processed", "both"]:
                processed_path = self.processed_path / f"{frame_uuid}.jpg"
                if processed_path.exists():
                    processed_path.unlink()
                    self.logger.debug(f"Deleted processed frame: {processed_path}")
                else:
                    self.logger.warning(f"Processed frame not found: {processed_path}")
            
            return success
            
        except Exception as e:
            self.logger.error(f"Error deleting frame {frame_uuid}: {e}")
            return False
    
    def delete_full_resolution_images(self, frame_uuid: str, keep_thumbnails: bool = True) -> bool:
        """
        Delete full-resolution images while optionally keeping thumbnails.
        This is useful after images have been processed by the image detector.
        
        Args:
            frame_uuid: Unique identifier for the frame
            keep_thumbnails: Whether to keep thumbnail versions
            
        Returns:
            True if successful, False otherwise
        """
        try:
            success = True
            
            # Delete full-resolution images
            original_path = self.original_path / f"{frame_uuid}.jpg"
            processed_path = self.processed_path / f"{frame_uuid}.jpg"
            
            if original_path.exists():
                original_path.unlink()
                self.logger.debug(f"Deleted full-resolution original image: {original_path}")
            else:
                self.logger.warning(f"Full-resolution original image not found: {original_path}")
            
            if processed_path.exists():
                processed_path.unlink()
                self.logger.debug(f"Deleted full-resolution processed image: {processed_path}")
            else:
                self.logger.warning(f"Full-resolution processed image not found: {processed_path}")
            
            # Optionally delete thumbnails too
            if not keep_thumbnails:
                self.delete_thumbnail(frame_uuid, "both")
                self.logger.debug(f"Deleted thumbnails for frame: {frame_uuid}")
            
            self.logger.info(f"Cleaned up full-resolution images for frame: {frame_uuid} (thumbnails: {'kept' if keep_thumbnails else 'deleted'})")
            return success
            
        except Exception as e:
            self.logger.error(f"Error deleting full-resolution images for frame {frame_uuid}: {e}")
            return False
    
    def frame_exists(self, frame_uuid: str, image_type: str = "processed") -> bool:
        """
        Check if a frame exists in local storage.
        
        Args:
            frame_uuid: Unique identifier for the frame
            image_type: Type of image ("original", "processed", or "both")
            
        Returns:
            True if frame exists, False otherwise
        """
        try:
            if image_type == "both":
                original_exists = (self.original_path / f"{frame_uuid}.jpg").exists()
                processed_exists = (self.processed_path / f"{frame_uuid}.jpg").exists()
                return original_exists and processed_exists
            elif image_type == "original":
                return (self.original_path / f"{frame_uuid}.jpg").exists()
            elif image_type == "processed":
                return (self.processed_path / f"{frame_uuid}.jpg").exists()
            else:
                raise ValueError(f"Invalid image_type: {image_type}")
                
        except Exception as e:
            self.logger.error(f"Error checking frame existence {frame_uuid}: {e}")
            return False
    
    def get_storage_stats(self) -> Dict[str, Any]:
        """
        Get storage statistics.
        
        Returns:
            Dictionary with storage statistics
        """
        try:
            stats = {
                "base_path": str(self.base_path),
                "original_path": str(self.original_path),
                "processed_path": str(self.processed_path),
                "original_count": 0,
                "processed_count": 0,
                "total_size_bytes": 0,
                "original_size_bytes": 0,
                "processed_size_bytes": 0
            }
            
            # Count original frames
            if self.original_path.exists():
                original_files = list(self.original_path.glob("*.jpg"))
                stats["original_count"] = len(original_files)
                stats["original_size_bytes"] = sum(f.stat().st_size for f in original_files)
            
            # Count processed frames
            if self.processed_path.exists():
                processed_files = list(self.processed_path.glob("*.jpg"))
                stats["processed_count"] = len(processed_files)
                stats["processed_size_bytes"] = sum(f.stat().st_size for f in processed_files)
            
            stats["total_size_bytes"] = stats["original_size_bytes"] + stats["processed_size_bytes"]
            
            return stats
            
        except Exception as e:
            self.logger.error(f"Error getting storage stats: {e}")
            return {}
    
    def cleanup_old_frames(self, max_age_days: int = 30) -> int:
        """
        Clean up old frame files.
        
        Args:
            max_age_days: Maximum age in days for frames to keep
            
        Returns:
            Number of files deleted
        """
        try:
            deleted_count = 0
            cutoff_time = datetime.now().timestamp() - (max_age_days * 24 * 60 * 60)
            
            # Clean up original frames
            if self.original_path.exists():
                for file_path in self.original_path.glob("*.jpg"):
                    if file_path.stat().st_mtime < cutoff_time:
                        file_path.unlink()
                        deleted_count += 1
                        self.logger.debug(f"Deleted old original frame: {file_path}")
            
            # Clean up processed frames
            if self.processed_path.exists():
                for file_path in self.processed_path.glob("*.jpg"):
                    if file_path.stat().st_mtime < cutoff_time:
                        file_path.unlink()
                        deleted_count += 1
                        self.logger.debug(f"Deleted old processed frame: {file_path}")
            
            if deleted_count > 0:
                self.logger.info(f"Cleaned up {deleted_count} old frame files")
            
            return deleted_count
            
        except Exception as e:
            self.logger.error(f"Error cleaning up old frames: {e}")
            return 0
