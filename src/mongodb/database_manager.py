"""
MongoDB Database Manager for Birds of Play

Handles database connections, collections, and common operations.
"""

import os
import logging
from typing import Optional, Dict, Any
from datetime import datetime
from pymongo import MongoClient
from pymongo.database import Database
from pymongo.collection import Collection


class DatabaseManager:
    """Manages MongoDB connections and provides access to collections."""
    
    def __init__(self, connection_string: str = "mongodb://localhost:27017/", 
                 database_name: str = "birds_of_play"):
        """
        Initialize the database manager.
        
        Args:
            connection_string: MongoDB connection string
            database_name: Name of the database to use
        """
        self.connection_string = connection_string
        self.database_name = database_name
        self.client: Optional[MongoClient] = None
        self.database: Optional[Database] = None
        self._collections: Dict[str, Collection] = {}
        
        # Setup logging
        self.logger = logging.getLogger(__name__)
        
    def connect(self) -> bool:
        """
        Establish connection to MongoDB.
        
        Returns:
            True if connection successful, False otherwise
        """
        try:
            self.client = MongoClient(self.connection_string)
            # Test the connection
            self.client.admin.command('ping')
            
            self.database = self.client[self.database_name]
            self.logger.info(f"Connected to MongoDB database: {self.database_name}")
            return True
            
        except Exception as e:
            self.logger.error(f"Failed to connect to MongoDB: {e}")
            return False
    
    def disconnect(self):
        """Close the MongoDB connection."""
        if self.client:
            self.client.close()
            self.logger.info("Disconnected from MongoDB")
    
    def get_collection(self, collection_name: str) -> Optional[Collection]:
        """
        Get a collection by name, creating it if it doesn't exist.
        
        Args:
            collection_name: Name of the collection
            
        Returns:
            Collection object or None if connection failed
        """
        if self.database is None:
            self.logger.error("Database not connected")
            return None
            
        if collection_name not in self._collections:
            self._collections[collection_name] = self.database[collection_name]
            self.logger.debug(f"Created collection: {collection_name}")
            
        return self._collections[collection_name]
    
    def is_connected(self) -> bool:
        """Check if connected to MongoDB."""
        return self.client is not None and self.database is not None
    
    def get_database_stats(self) -> Dict[str, Any]:
        """
        Get database statistics.
        
        Returns:
            Dictionary containing database statistics
        """
        if self.database is None:
            return {}
            
        try:
            stats = self.database.command("dbStats")
            return {
                "database": self.database_name,
                "collections": stats.get("collections", 0),
                "data_size": stats.get("dataSize", 0),
                "storage_size": stats.get("storageSize", 0),
                "indexes": stats.get("indexes", 0),
                "index_size": stats.get("indexSize", 0)
            }
        except Exception as e:
            self.logger.error(f"Failed to get database stats: {e}")
            return {}
    
    def __enter__(self):
        """Context manager entry."""
        self.connect()
        return self
    
    def __exit__(self, exc_type, exc_val, exc_tb):
        """Context manager exit."""
        self.disconnect()
