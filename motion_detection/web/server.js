const express = require('express');
const { MongoClient } = require('mongodb');
const moment = require('moment');
const cors = require('cors');
const path = require('path');

const app = express();
const port = 3000;

// MongoDB connection string
const uri = 'mongodb://localhost:27017';
const dbName = 'birds_of_play';

app.use(cors());
app.use(express.json());
app.use(express.static('public'));

// Connect to MongoDB
let db;
MongoClient.connect(uri)
    .then(client => {
        console.log('Connected to MongoDB');
        db = client.db(dbName);
    })
    .catch(err => {
        console.error('Error connecting to MongoDB:', err);
    });

// API Routes
app.get('/api/tracking/recent', async (req, res) => {
    try {
        const data = await db.collection('motion_tracking_data')
            .find({
                last_seen: {
                    $gt: new Date(Date.now() - 1000 * 60 * 60) // Last hour
                }
            })
            .sort({ last_seen: -1 })
            .toArray();
        
        res.json(data);
    } catch (err) {
        res.status(500).json({ error: err.message });
    }
});

app.get('/api/tracking/stats', async (req, res) => {
    try {
        const stats = await db.collection('motion_tracking_data').aggregate([
            {
                $group: {
                    _id: null,
                    total: { $sum: 1 },
                    avgConfidence: { $avg: '$confidence' },
                    maxTrajectoryLength: { $max: { $size: '$trajectory' } }
                }
            }
        ]).toArray();
        
        res.json(stats[0] || { total: 0, avgConfidence: 0, maxTrajectoryLength: 0 });
    } catch (err) {
        res.status(500).json({ error: err.message });
    }
});

app.get('/api/tracking/:uuid/image', async (req, res) => {
    try {
        const image = await db.collection('motion_tracking_images')
            .findOne({ uuid: req.params.uuid });
        
        if (!image) {
            res.status(404).json({ error: 'Image not found' });
            return;
        }
        
        res.set('Content-Type', 'image/png');
        res.send(Buffer.from(image.image.buffer));
    } catch (err) {
        res.status(500).json({ error: err.message });
    }
});

// New API endpoint: return all objects in the database
app.get('/api/tracking/all', async (req, res) => {
    try {
        const data = await db.collection('motion_tracking_data')
            .find({})
            .sort({ last_seen: -1 })
            .toArray();
        res.json(data);
    } catch (err) {
        res.status(500).json({ error: err.message });
    }
});

// Serve the main page
app.get('/', (req, res) => {
    res.sendFile(path.join(__dirname, 'public', 'index.html'));
});

// Serve the database view page
app.get('/database', (req, res) => {
    res.sendFile(path.join(__dirname, 'public', 'database.html'));
});

app.listen(port, () => {
    console.log(`Server running at http://localhost:${port}`);
}); 