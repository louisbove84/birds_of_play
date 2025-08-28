"""
Frame Database for Birds of Play

Handles storage and retrieval of captured frames with associated metadata.
"""

import uuid
import base64
import logging
from typing import Optional, Dict, Any, List
from datetime import datetime
import cv2
import numpy as np
from pymongo.collection import Collection

from .database_manager import DatabaseManager


class FrameDatabase:
    """Manages frame storage and retrieval in MongoDB."""
    
    def __init__(self, db_manager: DatabaseManager):
        """
        Initialize the frame database.
        
        Args:
            db_manager: Database manager instance
        """
        self.db_manager = db_manager
        self.collection_name = "captured_frames"
        self.logger = logging.getLogger(__name__)
        
    def _encode_frame(self, frame: np.ndarray) -> str:
        """
        Encode a frame as base64 string.
        
        Args:
            frame: OpenCV frame (numpy array)
            
        Returns:
            Base64 encoded string of the frame
        """
        try:
            # Encode frame as JPEG
            _, buffer = cv2.imencode('.jpg', frame)
            # Convert to base64
            encoded = base64.b64encode(buffer).decode('utf-8')
            return encoded
        except Exception as e:
            self.logger.error(f"Failed to encode frame: {e}")
            return ""
    
    def _decode_frame(self, encoded_frame: str) -> Optional[np.ndarray]:
        """
        Decode a base64 string back to frame.
        
        Args:
            encoded_frame: Base64 encoded frame string
            
        Returns:
            Decoded frame as numpy array or None if failed
        """
        try:
            # Decode base64
            buffer = base64.b64decode(encoded_frame)
            # Decode JPEG
            frame = cv2.imdecode(np.frombuffer(buffer, np.uint8), cv2.IMREAD_COLOR)
            return frame
        except Exception as e:
            self.logger.error(f"Failed to decode frame: {e}")
            return None
    
    def save_frame(self, frame: np.ndarray, metadata: Optional[Dict[str, Any]] = None) -> Optional[str]:
        """
        Save a frame to the database with a unique UUID.
        
        Args:
            frame: OpenCV frame to save
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
            
            # Encode frame
            encoded_frame = self._encode_frame(frame)
            if not encoded_frame:
                return None
            
            # Prepare document
            document = {
                "_id": frame_uuid,
                "frame_data": encoded_frame,
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
                self.logger.error("Failed to insert frame into database")
                return None
                
        except Exception as e:
            self.logger.error(f"Failed to save frame: {e}")
            return None
    
    def get_frame(self, frame_uuid: str) -> Optional[np.ndarray]:
        """
        Retrieve a frame by UUID.
        
        Args:
            frame_uuid: UUID of the frame to retrieve
            
        Returns:
            Frame as numpy array or None if not found
        """
        try:
            collection = self.db_manager.get_collection(self.collection_name)
            if collection is None:
                return None
            
            # Find document by UUID
            document = collection.find_one({"_id": frame_uuid})
            if not document:
                self.logger.warning(f"Frame not found with UUID: {frame_uuid}")
                return None
            
            # Decode frame
            encoded_frame = document.get("frame_data", "")
            frame = self._decode_frame(encoded_frame)
            
            if frame is not None:
                self.logger.info(f"Retrieved frame with UUID: {frame_uuid}")
            
            return frame
            
        except Exception as e:
            self.logger.error(f"Failed to retrieve frame {frame_uuid}: {e}")
            return None
    
    def get_frame_metadata(self, frame_uuid: str) -> Optional[Dict[str, Any]]:
        """
        Get metadata for a frame without loading the frame data.
        
        Args:
            frame_uuid: UUID of the frame
            
        Returns:
            Frame metadata or None if not found
        """
        try:
            collection = self.db_manager.get_collection(self.collection_name)
            if collection is None:
                return None
            
            # Find document by UUID, excluding frame data
            document = collection.find_one(
                {"_id": frame_uuid}, 
                {"frame_data": 0}  # Exclude frame data to save bandwidth
            )
            
            if not document:
                self.logger.warning(f"Frame metadata not found with UUID: {frame_uuid}")
                return None
            
            return document
            
        except Exception as e:
            self.logger.error(f"Failed to retrieve frame metadata {frame_uuid}: {e}")
            return None
    
    def list_frames(self, limit: int = 100, skip: int = 0) -> List[Dict[str, Any]]:
        """
        List frames in the database (metadata only, no frame data).
        
        Args:
            limit: Maximum number of frames to return
            skip: Number of frames to skip (for pagination)
            
        Returns:
            List of frame metadata dictionaries
        """
        try:
            collection = self.db_manager.get_collection(self.collection_name)
            if collection is None:
                return []
            
            # Find documents, excluding frame data
            cursor = collection.find(
                {}, 
                {"frame_data": 0}  # Exclude frame data to save bandwidth
            ).sort("timestamp", -1).skip(skip).limit(limit)
            
            frames = list(cursor)
            self.logger.info(f"Retrieved {len(frames)} frame metadata entries")
            return frames
            
        except Exception as e:
            self.logger.error(f"Failed to list frames: {e}")
            return []
    
    def delete_frame(self, frame_uuid: str) -> bool:
        """
        Delete a frame from the database.
        
        Args:
            frame_uuid: UUID of the frame to delete
            
        Returns:
            True if deleted successfully, False otherwise
        """
        try:
            collection = self.db_manager.get_collection(self.collection_name)
            if collection is None:
                return False
            
            result = collection.delete_one({"_id": frame_uuid})
            
            if result.deleted_count > 0:
                self.logger.info(f"Deleted frame with UUID: {frame_uuid}")
                return True
            else:
                self.logger.warning(f"Frame not found for deletion: {frame_uuid}")
                return False
                
        except Exception as e:
            self.logger.error(f"Failed to delete frame {frame_uuid}: {e}")
            return False
    
    def get_frame_count(self) -> int:
        """
        Get total number of frames in the database.
        
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
    
    def create_indexes(self):
        """Create database indexes for better performance."""
        try:
            collection = self.db_manager.get_collection(self.collection_name)
            if collection is None:
                return
            
            # Create indexes
            collection.create_index("timestamp")
            collection.create_index("created_at")
            collection.create_index("metadata.motion_detected")  # If motion detection metadata exists
            
            self.logger.info("Created database indexes")
            
        except Exception as e:
            self.logger.error(f"Failed to create indexes: {e}")
