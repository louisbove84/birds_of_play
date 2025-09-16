/**
 * Birds of Play - Image Detection Results Web Server
 * =================================================
 * 
 * This server provides a web interface for viewing image detection results
 * from the Birds of Play system. It displays detection results with navigation
 * back to original motion detection frames.
 */

const express = require('express');
const path = require('path');
const cors = require('cors');
const { MongoClient } = require('mongodb');

const app = express();
const PORT = process.env.PORT || 3001;

// MongoDB connection
const MONGODB_URI = process.env.MONGODB_URI || 'mongodb://localhost:27017';
const DB_NAME = 'birds_of_play';
const DETECTION_COLLECTION = 'detection_results';
const FRAME_COLLECTION = 'captured_frames';

let db;

// Middleware
app.use(cors());
app.use(express.json());

// Disable caching for development
app.use((req, res, next) => {
    res.set('Cache-Control', 'no-store, no-cache, must-revalidate, private');
    next();
});

app.use(express.static(path.join(__dirname, 'public')));

// Connect to MongoDB
async function connectToMongoDB() {
    try {
        const client = new MongoClient(MONGODB_URI);
        await client.connect();
        db = client.db(DB_NAME);
        console.log('✅ Connected to MongoDB successfully!');
        console.log(`📊 Database: ${MONGODB_URI}/${DB_NAME}`);
        console.log(`🔍 Detection collection: ${DETECTION_COLLECTION}`);
        console.log(`📷 Frame collection: ${FRAME_COLLECTION}`);
    } catch (error) {
        console.error('❌ MongoDB connection failed:', error);
        process.exit(1);
    }
}

// API Routes

// Get detection results summary
app.get('/api/detection-summary', async (req, res) => {
    try {
        const collection = db.collection(DETECTION_COLLECTION);
        
        const totalResults = await collection.countDocuments();
        const successfulResults = await collection.countDocuments({ processing_success: true });
        
        const pipeline = [
            {
                $group: {
                    _id: null,
                    totalDetections: { $sum: '$total_detections' },
                    totalRegions: { $sum: '$regions_processed' },
                    avgDetectionsPerFrame: { $avg: '$total_detections' }
                }
            }
        ];
        
        const stats = await collection.aggregate(pipeline).toArray();
        const statsData = stats[0] || {
            totalDetections: 0,
            totalRegions: 0,
            avgDetectionsPerFrame: 0
        };
        
        res.json({
            totalResults,
            successfulResults,
            successRate: totalResults > 0 ? successfulResults / totalResults : 0,
            totalDetections: statsData.totalDetections,
            totalRegions: statsData.totalRegions,
            avgDetectionsPerFrame: statsData.avgDetectionsPerFrame
        });
    } catch (error) {
        console.error('Error getting detection summary:', error);
        res.status(500).json({ error: 'Failed to get detection summary' });
    }
});

// Get detection results list
app.get('/api/detection-results', async (req, res) => {
    try {
        const { limit = 50, offset = 0, model = 'yolo11n' } = req.query;
        const collection = db.collection(DETECTION_COLLECTION);
        
        const query = model ? { detection_model: model } : {};
        
        const results = await collection
            .find(query)
            .sort({ processing_timestamp: -1 })
            .skip(parseInt(offset))
            .limit(parseInt(limit))
            .toArray();
        
        res.json(results);
    } catch (error) {
        console.error('Error getting detection results:', error);
        res.status(500).json({ error: 'Failed to get detection results' });
    }
});

// Get specific detection result
app.get('/api/detection-results/:frameUuid', async (req, res) => {
    try {
        const { frameUuid } = req.params;
        const { model = 'yolo11n' } = req.query;
        
        const collection = db.collection(DETECTION_COLLECTION);
        const result = await collection.findOne({
            frame_uuid: frameUuid,
            detection_model: model
        });
        
        if (!result) {
            return res.status(404).json({ error: 'Detection result not found' });
        }
        
        res.json(result);
    } catch (error) {
        console.error('Error getting detection result:', error);
        res.status(500).json({ error: 'Failed to get detection result' });
    }
});

// Get original frame data
app.get('/api/original-frame/:frameUuid', async (req, res) => {
    try {
        const { frameUuid } = req.params;
        const collection = db.collection(FRAME_COLLECTION);
        
        const frame = await collection.findOne({ _id: frameUuid });
        
        if (!frame) {
            return res.status(404).json({ error: 'Original frame not found' });
        }
        
        res.json({
            uuid: frame._id,
            timestamp: frame.timestamp,
            metadata: frame.metadata,
            imageData: frame.frame_data
        });
    } catch (error) {
        console.error('Error getting original frame:', error);
        res.status(500).json({ error: 'Failed to get original frame' });
    }
});

// Get frame image (processed frame with overlays)
app.get('/api/frame-image/:frameUuid', async (req, res) => {
    try {
        const { frameUuid } = req.params;
        console.log(`📷 Requesting frame image for UUID: ${frameUuid}`);
        const collection = db.collection(FRAME_COLLECTION);
        
        const frame = await collection.findOne({ _id: frameUuid });
        console.log(`📊 Frame found: ${!!frame}, has frame_data: ${!!(frame && frame.frame_data)}`);
        
        if (!frame || !frame.frame_data) {
            console.log(`❌ Frame image not found for UUID: ${frameUuid}`);
            return res.status(404).json({ error: 'Frame image not found' });
        }
        
        // Set appropriate headers for image
        res.set('Content-Type', 'image/jpeg');
        res.set('Cache-Control', 'no-cache');
        
        // Send binary image data
        res.send(Buffer.from(frame.frame_data, 'base64'));
    } catch (error) {
        console.error('Error getting frame image:', error);
        res.status(500).json({ error: 'Failed to get frame image' });
    }
});

// Get original frame image (clean frame without overlays)
app.get('/api/original-frame-image/:frameUuid', async (req, res) => {
    try {
        const { frameUuid } = req.params;
        console.log(`📸 Requesting ORIGINAL frame image for UUID: ${frameUuid}`);
        const collection = db.collection(FRAME_COLLECTION);
        
        const frame = await collection.findOne({ _id: frameUuid });
        console.log(`📊 Frame found: ${!!frame}, has original_frame_data: ${!!(frame && frame.original_frame_data)}, has frame_data: ${!!(frame && frame.frame_data)}`);
        
        if (!frame) {
            return res.status(404).json({ error: 'Frame not found' });
        }
        
        // Prefer original frame data, fallback to processed frame data
        const imageData = frame.original_frame_data || frame.frame_data;
        if (!imageData) {
            console.log(`❌ No image data found for UUID: ${frameUuid}`);
            return res.status(404).json({ error: 'Original frame image not found' });
        }
        
        console.log(`✅ Serving ${frame.original_frame_data ? 'ORIGINAL' : 'PROCESSED'} frame for UUID: ${frameUuid}`);
        
        // Set appropriate headers for image
        res.set('Content-Type', 'image/jpeg');
        res.set('Cache-Control', 'no-cache');
        
        // Send binary image data
        res.send(Buffer.from(imageData, 'base64'));
    } catch (error) {
        console.error('Error getting original frame image:', error);
        res.status(500).json({ error: 'Failed to get original frame image' });
    }
});

// Get detection result image with overlays
app.get('/api/detection-image/:frameUuid', async (req, res) => {
    try {
        const { frameUuid } = req.params;
        const { model = 'yolo11n' } = req.query;
        
        // Get detection result
        const detectionCollection = db.collection(DETECTION_COLLECTION);
        const detectionResult = await detectionCollection.findOne({
            frame_uuid: frameUuid,
            detection_model: model
        });
        
        if (!detectionResult) {
            return res.status(404).json({ error: 'Detection result not found' });
        }
        
        // Get original frame
        const frameCollection = db.collection(FRAME_COLLECTION);
        const frame = await frameCollection.findOne({ _id: frameUuid });
        
        if (!frame) {
            return res.status(404).json({ error: 'Original frame not found' });
        }
        
        // Use original frame data if available, otherwise fall back to processed frame
        const imageData = frame.original_frame_data || frame.frame_data;
        if (!imageData) {
            return res.status(404).json({ error: 'Frame image not found' });
        }
        
        // TODO: Implement image overlay generation with detection results
        // For now, return the original image
        res.set('Content-Type', 'image/jpeg');
        res.set('Cache-Control', 'no-cache');
        res.send(Buffer.from(imageData, 'base64'));
        
    } catch (error) {
        console.error('Error getting detection image:', error);
        res.status(500).json({ error: 'Failed to get detection image' });
    }
});

// Serve main page
app.get('/', (req, res) => {
    res.sendFile(path.join(__dirname, 'public', 'index.html'));
});

// Start server
async function startServer() {
    await connectToMongoDB();
    
    app.listen(PORT, () => {
        console.log(`🚀 Birds of Play Detection Results Server running on http://localhost:${PORT}`);
        console.log(`📊 MongoDB: ${MONGODB_URI}/${DB_NAME}`);
        console.log(`🔍 Detection Results: http://localhost:${PORT}`);
    });
}

startServer().catch(console.error);
