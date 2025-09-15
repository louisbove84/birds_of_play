const express = require('express');
const { MongoClient } = require('mongodb');
const path = require('path');
const cors = require('cors');

const app = express();
const PORT = process.env.PORT || 3000;

// MongoDB connection
const MONGODB_URI = 'mongodb://localhost:27017';
const DB_NAME = 'birds_of_play';
const COLLECTION_NAME = 'captured_frames';

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

// Set EJS as templating engine
app.set('view engine', 'ejs');
app.set('views', path.join(__dirname, 'views'));

// Connect to MongoDB
async function connectToMongoDB() {
    try {
        const client = new MongoClient(MONGODB_URI);
        await client.connect();
        db = client.db(DB_NAME);
        console.log('✅ Connected to MongoDB successfully!');
    } catch (error) {
        console.error('❌ MongoDB connection error:', error);
    }
}

// Routes
app.get('/', async (req, res) => {
    try {
        const collection = db.collection(COLLECTION_NAME);
        const frames = await collection.find({}).sort({ 'metadata.timestamp': -1 }).limit(50).toArray();
        
        res.render('index', { 
            frames: frames,
            totalFrames: frames.length
        });
    } catch (error) {
        console.error('Error fetching frames:', error);
        res.render('index', { frames: [], totalFrames: 0, error: error.message });
    }
});

// API Routes
app.get('/api/frames', async (req, res) => {
    try {
        const collection = db.collection(COLLECTION_NAME);
        const page = parseInt(req.query.page) || 1;
        const limit = parseInt(req.query.limit) || 20;
        const skip = (page - 1) * limit;
        
        const frames = await collection.find({})
            .sort({ 'metadata.timestamp': -1 })
            .skip(skip)
            .limit(limit)
            .toArray();
            
        const total = await collection.countDocuments();
        
        // Transform frames to match frontend expectations
        const transformedFrames = frames.map(frame => ({
            ...frame,
            image_data: frame.frame_data || null  // Map frame_data to image_data
        }));
        
        res.json({
            frames: transformedFrames,
            pagination: {
                page: page,
                limit: limit,
                total: total,
                pages: Math.ceil(total / limit)
            }
        });
    } catch (error) {
        res.status(500).json({ error: error.message });
    }
});

app.get('/api/frames/:id', async (req, res) => {
    try {
        const collection = db.collection(COLLECTION_NAME);
        const frame = await collection.findOne({ _id: req.params.id });
        
        if (!frame) {
            return res.status(404).json({ error: 'Frame not found' });
        }
        
        // Transform frame to match frontend expectations
        const transformedFrame = {
            ...frame,
            image_data: frame.frame_data || null  // Map frame_data to image_data
        };
        
        res.json(transformedFrame);
    } catch (error) {
        res.status(500).json({ error: error.message });
    }
});

app.get('/api/stats', async (req, res) => {
    try {
        const collection = db.collection(COLLECTION_NAME);
        const totalFrames = await collection.countDocuments();
        const latestFrame = await collection.findOne({}, { sort: { 'metadata.timestamp': -1 } });
        const oldestFrame = await collection.findOne({}, { sort: { 'metadata.timestamp': 1 } });
        
        res.json({
            totalFrames: totalFrames,
            latestFrame: latestFrame?.metadata?.timestamp,
            oldestFrame: oldestFrame?.metadata?.timestamp
        });
    } catch (error) {
        res.status(500).json({ error: error.message });
    }
});

// Start server
async function startServer() {
    await connectToMongoDB();
    
    app.listen(PORT, () => {
        console.log(`🚀 Birds of Play Web Viewer running on http://localhost:${PORT}`);
        console.log(`📊 MongoDB: ${MONGODB_URI}/${DB_NAME}/${COLLECTION_NAME}`);
    });
}

startServer().catch(console.error);
