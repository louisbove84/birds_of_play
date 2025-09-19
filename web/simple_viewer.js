#!/usr/bin/env node

const express = require('express');
const { MongoClient } = require('mongodb');
const path = require('path');
const fs = require('fs');

const app = express();
const PORT = 3000;

let db = null;

// Connect to MongoDB
async function connectToMongoDB() {
    try {
        const client = new MongoClient('mongodb://localhost:27017');
        await client.connect();
        db = client.db('birds_of_play');
        console.log('✅ Connected to MongoDB successfully!');
        return true;
    } catch (error) {
        console.error('❌ MongoDB connection failed:', error);
        return false;
    }
}

// Serve static files
app.use(express.static(path.join(__dirname, 'public')));
app.use('/images', express.static(path.join(__dirname, 'data/frames')));

// Main page
app.get('/', (req, res) => {
    res.send(`
<!DOCTYPE html>
<html>
<head>
    <title>Birds of Play - Simple Viewer</title>
    <style>
        body { 
            font-family: Arial, sans-serif; 
            margin: 20px; 
            background: #1a1a1a; 
            color: white; 
        }
        .container { max-width: 1200px; margin: 0 auto; }
        h1 { color: #FF6B35; text-align: center; }
        .frame-grid { 
            display: grid; 
            grid-template-columns: repeat(auto-fill, minmax(300px, 1fr)); 
            gap: 20px; 
            margin-top: 20px; 
        }
        .frame-card { 
            background: #2a2a2a; 
            border-radius: 8px; 
            padding: 15px; 
            border: 1px solid #444; 
        }
        .frame-card img { 
            width: 100%; 
            height: 200px; 
            object-fit: cover; 
            border-radius: 4px; 
            cursor: pointer;
            border: 2px solid transparent;
        }
        .frame-card img:hover { border-color: #4CAF50; }
        .frame-info { margin-top: 10px; font-size: 12px; color: #ccc; }
        .frame-actions { margin-top: 10px; text-align: center; }
        .btn { 
            background: #4CAF50; 
            color: white; 
            border: none; 
            padding: 5px 10px; 
            border-radius: 4px; 
            cursor: pointer; 
            font-size: 11px;
            margin: 2px;
        }
        .btn:hover { background: #45a049; }
        .btn.secondary { background: #666; }
        .btn.secondary:hover { background: #777; }
        .loading { text-align: center; padding: 50px; }
        .modal {
            display: none;
            position: fixed;
            top: 0; left: 0;
            width: 100%; height: 100%;
            background: rgba(0,0,0,0.9);
            z-index: 1000;
        }
        .modal-content {
            position: absolute;
            top: 50%; left: 50%;
            transform: translate(-50%, -50%);
            max-width: 90%; max-height: 90%;
        }
        .modal img { max-width: 100%; max-height: 100%; }
        .close { 
            position: absolute; 
            top: 10px; right: 20px; 
            color: white; 
            font-size: 30px; 
            cursor: pointer; 
        }
        .stats { text-align: center; margin-bottom: 20px; color: #4CAF50; }
        .nav-links {
            text-align: center;
            margin: 20px 0;
            padding: 10px;
            background: #333;
            border-radius: 8px;
        }
        .nav-link {
            color: #4CAF50;
            text-decoration: none;
            padding: 8px 16px;
            border-radius: 4px;
            transition: all 0.3s ease;
            font-weight: bold;
        }
        .nav-link:hover {
            background: #4CAF50;
            color: #1a1a1a;
        }
        .nav-current {
            color: #FF6B35;
            font-weight: bold;
            padding: 8px 16px;
        }
        .nav-separator {
            color: #666;
            margin: 0 10px;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🐦 Birds of Play - Motion Detection Viewer</h1>
        <p style="text-align: center; color: #ccc;">Consolidated motion regions with original thumbnails</p>
        
        <nav class="nav-links">
            <span class="nav-current">📹 Motion Detection Frames</span>
            <span class="nav-separator">|</span>
            <a href="http://localhost:3001" class="nav-link" target="_self">
                🎯 Object Detections
            </a>
            <span class="nav-separator">|</span>
            <a href="http://localhost:3002/dashboard" class="nav-link" target="_self">
                🔬 Bird Clustering
            </a>
            <span class="nav-separator">|</span>
            <a href="http://localhost:3003" class="nav-link" target="_self">
                🧠 Fine-Tuning
            </a>
        </nav>
        
        <div class="stats" id="stats">Loading...</div>
        <div id="frames" class="loading">Loading frames...</div>
    </div>

    <!-- Modal for full-size image -->
    <div id="modal" class="modal" onclick="closeModal()">
        <span class="close">&times;</span>
        <div class="modal-content">
            <img id="modal-img" src="" alt="Full size frame">
        </div>
    </div>

    <script>
        async function loadFrames() {
            try {
                const response = await fetch('/api/frames');
                const data = await response.json();
                
                document.getElementById('stats').innerHTML = 
                    \`📊 Total Frames: \${data.frames.length} | 🎯 Motion Detected\`;
                
                const framesContainer = document.getElementById('frames');
                framesContainer.innerHTML = '';
                framesContainer.className = 'frame-grid';
                
                data.frames.forEach(frame => {
                    const card = document.createElement('div');
                    card.className = 'frame-card';
                    
                    const timestamp = new Date(frame.timestamp).toLocaleString();
                    const regions = frame.metadata?.consolidated_regions?.length || 0;
                    
                    card.innerHTML = \`
                        <img id="img-\${frame._id}" src="/api/image/\${frame._id}" 
                             alt="Frame \${frame._id}" 
                             onclick="openModal('/api/image/\${frame._id}')"
                             onerror="this.src='/api/fallback/\${frame._id}'">
                        <div class="frame-info">
                            <strong>ID:</strong> \${frame._id}<br>
                            <strong>Time:</strong> \${timestamp}<br>
                            <strong>Regions:</strong> \${regions} consolidated regions<br>
                            <strong>Motion:</strong> \${frame.metadata?.motion_regions || 0} detections
                        </div>
                        <div class="frame-actions">
                            <button class="btn" onclick="toggleImage('\${frame._id}', 'processed')">🔍 Processed</button>
                            <button class="btn secondary" onclick="toggleImage('\${frame._id}', 'original')">📷 Original</button>
                        </div>
                    \`;
                    
                    framesContainer.appendChild(card);
                });
                
            } catch (error) {
                console.error('Error loading frames:', error);
                document.getElementById('frames').innerHTML = 
                    '<div style="color: red; text-align: center;">Error loading frames. Check console for details.</div>';
            }
        }
        
        function openModal(imageSrc) {
            document.getElementById('modal-img').src = imageSrc;
            document.getElementById('modal').style.display = 'block';
        }
        
        function closeModal() {
            document.getElementById('modal').style.display = 'none';
        }
        
        function toggleImage(frameId, type) {
            const img = document.getElementById(\`img-\${frameId}\`);
            if (!img) return;
            
            // Update button states
            const buttons = img.parentElement.querySelectorAll('.btn');
            buttons.forEach(btn => {
                btn.classList.remove('btn', 'secondary');
                btn.classList.add('btn', 'secondary');
            });
            
            // Highlight active button
            const activeBtn = img.parentElement.querySelector(\`button[onclick*="\${type}"]\`);
            if (activeBtn) {
                activeBtn.classList.remove('secondary');
            }
            
            // Update image source
            if (type === 'original') {
                img.src = \`/api/original/\${frameId}\`;
            } else {
                img.src = \`/api/image/\${frameId}\`;
            }
        }
        
        // Load frames when page loads
        window.addEventListener('load', loadFrames);
        
        // Close modal with Escape key
        document.addEventListener('keydown', function(e) {
            if (e.key === 'Escape') closeModal();
        });
    </script>
</body>
</html>
    `);
});

// API endpoint to get frames
app.get('/api/frames', async (req, res) => {
    try {
        const collection = db.collection('captured_frames');
        const frames = await collection.find({})
            .sort({ timestamp: -1 })
            .limit(50)
            .toArray();
            
        res.json({ frames });
    } catch (error) {
        console.error('Error fetching frames:', error);
        res.status(500).json({ error: 'Failed to fetch frames' });
    }
});

// API endpoint to serve images
app.get('/api/image/:frameId', async (req, res) => {
    try {
        const { frameId } = req.params;
        const collection = db.collection('captured_frames');
        const frame = await collection.findOne({ _id: frameId });
        
        if (!frame) {
            return res.status(404).send('Frame not found');
        }
        
        // Try processed image first, then original
        const imagePath = frame.processed_image_path || frame.original_image_path;
        if (!imagePath) {
            return res.status(404).send('No image path found');
        }
        
        // Resolve absolute path - go up one level from web/ to project root
        const absolutePath = path.resolve(__dirname, '..', imagePath);
        
        if (!fs.existsSync(absolutePath)) {
            console.log(`❌ Image not found: ${absolutePath}`);
            return res.status(404).send('Image file not found');
        }
        
        console.log(`✅ Serving image: ${frameId}`);
        res.sendFile(absolutePath);
        
    } catch (error) {
        console.error('Error serving image:', error);
        res.status(500).send('Error serving image');
    }
});

// API endpoint to serve original images (without motion detection overlays)
app.get('/api/original/:frameId', async (req, res) => {
    try {
        const { frameId } = req.params;
        const collection = db.collection('captured_frames');
        const frame = await collection.findOne({ _id: frameId });
        
        if (!frame) {
            return res.status(404).send('Frame not found');
        }
        
        // Use original image path (without motion detection overlays)
        const imagePath = frame.original_image_path;
        if (!imagePath) {
            return res.status(404).send('No original image path found');
        }
        
        // Resolve absolute path - go up one level from web/ to project root
        const absolutePath = path.resolve(__dirname, '..', imagePath);
        
        if (!fs.existsSync(absolutePath)) {
            console.log(`❌ Original image not found: ${absolutePath}`);
            return res.status(404).send('Original image file not found');
        }
        
        console.log(`✅ Serving original image: ${frameId}`);
        res.sendFile(absolutePath);
        
    } catch (error) {
        console.error('Error serving original image:', error);
        res.status(500).send('Error serving original image');
    }
});

// Fallback endpoint for missing images
app.get('/api/fallback/:frameId', (req, res) => {
    // Create a simple placeholder image
    res.send(`
        <svg width="300" height="200" xmlns="http://www.w3.org/2000/svg">
            <rect width="100%" height="100%" fill="#333"/>
            <text x="50%" y="50%" text-anchor="middle" fill="white" font-family="Arial">
                Image not found
            </text>
        </svg>
    `);
    res.type('image/svg+xml');
});

// Start server
async function startServer() {
    const connected = await connectToMongoDB();
    if (!connected) {
        console.error('❌ Cannot start server without MongoDB connection');
        process.exit(1);
    }
    
    app.listen(PORT, () => {
        console.log(`🚀 Simple Birds of Play Viewer running on http://localhost:${PORT}`);
        console.log('📊 MongoDB: mongodb://localhost:27017/birds_of_play');
        console.log('🎯 Ready to view motion detection frames!');
    });
}

startServer();
