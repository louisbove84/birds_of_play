"""
Detection Results Database Manager
=================================

This module provides MongoDB integration for storing and retrieving
image detection results.
"""

from typing import List, Dict, Any, Optional
from datetime import datetime, timezone
from .detection_result import FrameDetectionResult, DetectionSummary


class DetectionDatabase:
    """MongoDB database manager for detection results"""
    
    def __init__(self, db_manager):
        """
        Initialize detection database
        
        Args:
            db_manager: MongoDB database manager instance
        """
        self.db_manager = db_manager
        self.collection_name = "detection_results"
        self.collection = None
        self._initialize_collection()
    
    def _initialize_collection(self):
        """Initialize the detection results collection"""
        try:
            self.collection = self.db_manager.database[self.collection_name]
            self._create_indexes()
            print(f"✅ Detection results collection initialized: {self.collection_name}")
        except Exception as e:
            print(f"❌ Failed to initialize detection collection: {e}")
            raise
    
    def _create_indexes(self):
        """Create database indexes for better performance"""
        try:
            # Index on frame_uuid for fast lookups
            self.collection.create_index("frame_uuid")
            
            # Index on detection_model for filtering
            self.collection.create_index("detection_model")
            
            # Index on processing_timestamp for time-based queries
            self.collection.create_index("processing_timestamp")
            
            # Compound index for common queries
            self.collection.create_index([
                ("frame_uuid", 1),
                ("detection_model", 1)
            ])
            
            print("✅ Detection results indexes created")
        except Exception as e:
            print(f"⚠️  Warning: Could not create indexes: {e}")
    
    def store_detection_result(self, result: FrameDetectionResult) -> bool:
        """
        Store a single frame detection result
        
        Args:
            result: Frame detection result to store
            
        Returns:
            True if successful, False otherwise
        """
        try:
            # Convert to MongoDB document format
            detection_doc = {
                'frame_uuid': result.frame_uuid,
                'original_frame_metadata': result.original_frame_metadata,
                'detection_model': result.detection_model,
                'processing_timestamp': result.processing_timestamp,
                'regions_processed': result.regions_processed,
                'total_detections': result.total_detections,
                'processing_success': result.processing_success,
                'processing_error': result.processing_error,
                'region_results': [
                    {
                        'region_id': region.region_id,
                        'region_bounds': region.region_bounds,
                        'detection_count': region.detection_count,
                        'processed': region.processed,
                        'processing_error': region.processing_error,
                        'processing_timestamp': region.processing_timestamp,
                        'detections': [
                            {
                                'class_id': det.class_id,
                                'class_name': det.class_name,
                                'confidence': det.confidence,
                                'bbox_x1': det.bbox_x1,
                                'bbox_y1': det.bbox_y1,
                                'bbox_x2': det.bbox_x2,
                                'bbox_y2': det.bbox_y2,
                                'region_x': det.region_x,
                                'region_y': det.region_y,
                                'region_w': det.region_w,
                                'region_h': det.region_h,
                                'detection_timestamp': det.detection_timestamp
                            }
                            for det in region.detections
                        ]
                    }
                    for region in result.region_results
                ]
            }
            
            # Insert document
            insert_result = self.collection.insert_one(detection_doc)
            
            if insert_result.inserted_id:
                print(f"   📄 Stored detection results for frame {result.frame_uuid[:8]}... ({result.total_detections} detections)")
                return True
            else:
                print(f"   ❌ Failed to store detection results for frame {result.frame_uuid[:8]}...")
                return False
                
        except Exception as e:
            print(f"   ❌ Error storing detection result: {e}")
            return False
    
    def store_detection_results(self, results: List[FrameDetectionResult]) -> int:
        """
        Store multiple frame detection results
        
        Args:
            results: List of frame detection results to store
            
        Returns:
            Number of successfully stored results
        """
        print("💾 Storing detection results in MongoDB...")
        
        stored_count = 0
        for result in results:
            if self.store_detection_result(result):
                stored_count += 1
        
        print(f"✅ Stored {stored_count}/{len(results)} detection results")
        return stored_count
    
    def get_detection_result(self, frame_uuid: str, detection_model: str = "yolo11n") -> Optional[Dict[str, Any]]:
        """
        Get detection result for a specific frame
        
        Args:
            frame_uuid: UUID of the frame
            detection_model: Detection model used
            
        Returns:
            Detection result document or None if not found
        """
        try:
            result = self.collection.find_one({
                "frame_uuid": frame_uuid,
                "detection_model": detection_model
            })
            return result
        except Exception as e:
            print(f"❌ Error retrieving detection result: {e}")
            return None
    
    def list_detection_results(self, limit: int = 100, detection_model: str = None) -> List[Dict[str, Any]]:
        """
        List detection results
        
        Args:
            limit: Maximum number of results to return
            detection_model: Filter by detection model (optional)
            
        Returns:
            List of detection result documents
        """
        try:
            query = {}
            if detection_model:
                query["detection_model"] = detection_model
            
            results = list(self.collection.find(query).limit(limit))
            return results
        except Exception as e:
            print(f"❌ Error listing detection results: {e}")
            return []
    
    def get_detection_count(self) -> int:
        """Get total number of detection results"""
        try:
            return self.collection.count_documents({})
        except Exception as e:
            print(f"❌ Error getting detection count: {e}")
            return 0
    
    def get_detection_summary(self, detection_model: str = None) -> Dict[str, Any]:
        """
        Get summary statistics for detection results
        
        Args:
            detection_model: Filter by detection model (optional)
            
        Returns:
            Summary statistics dictionary
        """
        try:
            query = {}
            if detection_model:
                query["detection_model"] = detection_model
            
            # Get basic counts
            total_results = self.collection.count_documents(query)
            successful_results = self.collection.count_documents({**query, "processing_success": True})
            
            # Get detection counts
            pipeline = [
                {"$match": query},
                {"$group": {
                    "_id": None,
                    "total_detections": {"$sum": "$total_detections"},
                    "total_regions": {"$sum": "$regions_processed"},
                    "avg_detections_per_frame": {"$avg": "$total_detections"}
                }}
            ]
            
            stats = list(self.collection.aggregate(pipeline))
            if stats:
                stats = stats[0]
            else:
                stats = {
                    "total_detections": 0,
                    "total_regions": 0,
                    "avg_detections_per_frame": 0
                }
            
            return {
                "total_results": total_results,
                "successful_results": successful_results,
                "success_rate": successful_results / total_results if total_results > 0 else 0.0,
                "total_detections": stats["total_detections"],
                "total_regions": stats["total_regions"],
                "avg_detections_per_frame": stats["avg_detections_per_frame"]
            }
            
        except Exception as e:
            print(f"❌ Error getting detection summary: {e}")
            return {}
    
    def clear_detection_results(self, detection_model: str = None) -> int:
        """
        Clear detection results
        
        Args:
            detection_model: Clear only results for specific model (optional)
            
        Returns:
            Number of documents deleted
        """
        try:
            query = {}
            if detection_model:
                query["detection_model"] = detection_model
            
            result = self.collection.delete_many(query)
            deleted_count = result.deleted_count
            
            if deleted_count > 0:
                print(f"✅ Cleared {deleted_count} detection results")
            else:
                print("📭 No detection results to clear")
            
            return deleted_count
            
        except Exception as e:
            print(f"❌ Error clearing detection results: {e}")
            return 0
