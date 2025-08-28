# MongoDB Package for Birds of Play

This package provides MongoDB database functionality for the Birds of Play motion detection system.

## Structure

```
src/mongodb/
├── __init__.py              # Package initialization
├── database_manager.py      # Core database connection management
├── frame_database.py        # Frame storage and retrieval
├── test_frame_database.py   # Test script for frame database
└── README.md               # This file
```

## Features

### Database Manager (`database_manager.py`)
- **Connection Management**: Handles MongoDB connections with error handling
- **Collection Access**: Provides easy access to database collections
- **Context Manager**: Supports `with` statements for automatic cleanup
- **Database Statistics**: Get database stats and performance metrics

### Frame Database (`frame_database.py`)
- **Frame Storage**: Save OpenCV frames with unique UUIDs
- **Base64 Encoding**: Efficient frame encoding/decoding for MongoDB storage
- **Metadata Support**: Store additional metadata with each frame
- **Retrieval Operations**: Get frames by UUID, list frames, get metadata
- **Performance Indexes**: Automatic index creation for better query performance

## Usage

### Basic Setup

```python
from mongodb.database_manager import DatabaseManager
from mongodb.frame_database import FrameDatabase

# Initialize database manager
db_manager = DatabaseManager()

# Connect to database
if db_manager.connect():
    # Initialize frame database
    frame_db = FrameDatabase(db_manager)
    
    # Create indexes for better performance
    frame_db.create_indexes()
```

### Saving Frames

```python
import cv2
import numpy as np

# Create or capture a frame
frame = cv2.imread('image.jpg')  # or from video capture

# Prepare metadata
metadata = {
    "source": "webcam",
    "motion_detected": True,
    "motion_regions": 5,
    "confidence": 0.85
}

# Save frame with UUID
frame_uuid = frame_db.save_frame(frame, metadata)
print(f"Saved frame with UUID: {frame_uuid}")
```

### Retrieving Frames

```python
# Get frame by UUID
retrieved_frame = frame_db.get_frame(frame_uuid)

# Get metadata only (faster, no frame data)
metadata = frame_db.get_frame_metadata(frame_uuid)

# List recent frames
recent_frames = frame_db.list_frames(limit=10, skip=0)
```

### Database Statistics

```python
# Get frame count
count = frame_db.get_frame_count()

# Get database stats
stats = db_manager.get_database_stats()
print(f"Database: {stats['database']}")
print(f"Collections: {stats['collections']}")
print(f"Data size: {stats['data_size']} bytes")
```

## Testing

Run the test script to verify functionality:

```bash
cd src/mongodb
python test_frame_database.py
```

This will test:
- Database connection
- Frame saving and retrieval
- Metadata operations
- Frame deletion
- Database statistics

## Database Schema

### Frame Collection (`captured_frames`)

```json
{
  "_id": "uuid-string",
  "frame_data": "base64-encoded-jpeg",
  "frame_shape": [height, width, channels],
  "frame_dtype": "uint8",
  "timestamp": "2024-01-01T12:00:00Z",
  "created_at": "2024-01-01T12:00:00Z",
  "metadata": {
    "source": "webcam",
    "motion_detected": true,
    "motion_regions": 5,
    "confidence": 0.85
  }
}
```

## Performance Considerations

- **Indexes**: Automatic creation of indexes on `timestamp`, `created_at`, and metadata fields
- **Base64 Encoding**: Efficient JPEG compression before storage
- **Metadata Queries**: Fast queries without loading frame data
- **Pagination**: Support for large result sets with skip/limit

## Error Handling

All operations include comprehensive error handling:
- Connection failures
- Encoding/decoding errors
- Database operation failures
- Invalid UUIDs

## Dependencies

- `pymongo>=4.0.0`: MongoDB driver
- `opencv-python>=4.5.0`: Image processing
- `numpy>=1.21.0`: Numerical operations

## Future Enhancements

- **Batch Operations**: Save multiple frames at once
- **Compression Options**: Different compression algorithms
- **Query Filters**: Advanced filtering by metadata
- **Backup/Restore**: Database backup functionality
