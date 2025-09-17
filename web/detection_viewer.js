#!/usr/bin/env node

const express = require('express');
const { MongoClient } = require('mongodb');
const path = require('path');
const fs = require('fs');

const app = express();
const PORT = 3002;

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

// Main page
app.get('/', (req, res) => {
    res.send(`
<!DOCTYPE html>
<html>
<head>
    <title>Birds of Play - Motion Detection with Object Detection</title>
    <style>
        body { 
            font-family: Arial, sans-serif; 
            margin: 20px; 
            background: #1a1a1a; 
            color: white; 
        }
        .container { max-width: 1200px; margin: 0 auto; }
        h1 { color: #FF6B35; text-align: center; }
        .nav-links {
            margin-top: 1rem;
            font-size: 0.9em;
            background: rgba(255, 255, 255, 0.1);
            padding: 0.5rem 1rem;
            border-radius: 8px;
            display: inline-block;
        }
        .nav-link {
            color: #00FF00;
            text-decoration: none;
            padding: 0.5rem 1rem;
            border-radius: 4px;
            transition: all 0.3s ease;
            border: 2px solid #00FF00;
            background-color: rgba(0, 255, 0, 0.1);
            font-weight: bold;
            text-shadow: 1px 1px 2px rgba(0, 0, 0, 0.8);
        }
        .nav-link:hover {
            background-color: rgba(0, 255, 0, 0.3);
            text-decoration: underline;
            border-color: #00FF00;
            transform: translateY(-1px);
            box-shadow: 0 2px 8px rgba(0, 255, 0, 0.4);
        }
        .nav-separator {
            margin: 0 1rem;
            color: #666;
            font-weight: bold;
        }
        .nav-current {
            color: #FFC107;
            font-weight: bold;
            background: rgba(255, 193, 7, 0.1);
            padding: 0.5rem 1rem;
            border-radius: 4px;
            border: 1px solid rgba(255, 193, 7, 0.3);
        }
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
        .frame-card img:hover { border-color: #FF6B35; }
        .frame-info { margin-top: 10px; font-size: 12px; color: #ccc; }
        .frame-actions { margin-top: 10px; text-align: center; }
        .btn { 
            background: #FF6B35; 
            color: white; 
            border: none; 
            padding: 5px 10px; 
            border-radius: 4px; 
            cursor: pointer; 
            font-size: 11px;
            margin: 2px;
        }
        .btn:hover { background: #E55A2B; }
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
        .stats { text-align: center; margin-bottom: 20px; color: #FF6B35; }
    </style>
</head>
<body>
    <div class="container">
        <h1>🎯 Birds of Play - Motion Detection with Object Detection</h1>
        <p style="text-align: center; color: #ccc;">Combined view of motion detection regions and YOLO11 object detection results</p>
        
        <nav class="nav-links">
            <a href="http://localhost:3000" class="nav-link">
                📹 Motion Detection Only
            </a>
            <span class="nav-separator">|</span>
            <span class="nav-current">🎯 Motion + Object Detection</span>
            <span class="nav-separator">|</span>
            <a href="http://localhost:3001" class="nav-link">
                🔍 Object Detection Results
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
                    \`📊 Total Frames: \${data.frames.length} | 🎯 Motion + Object Detection\`;
                
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
                            <button class="btn" onclick="toggleImage('\${frame._id}', 'processed')">🔍 Motion Only</button>
                            <button class="btn secondary" onclick="toggleImage('\${frame._id}', 'original')">📷 Original</button>
                            <button class="btn secondary" onclick="toggleImage('\${frame._id}', 'detection')">🎯 Motion + Objects</button>
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
            } else if (type === 'detection') {
                img.src = \`/api/detection-overlay/\${frameId}\`;
            } else {
                img.src = \`/api/image/\${frameId}\`;
            }
        }
        
        function openModal(imageSrc) {
            document.getElementById('modal-img').src = imageSrc;
            document.getElementById('modal').style.display = 'block';
        }
        
        function closeModal() {
            document.getElementById('modal').style.display = 'none';
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

// API endpoint to serve images with YOLO11 detection overlays
app.get('/api/detection-overlay/:frameId', async (req, res) => {
    try {
        const { frameId } = req.params;
        const frameCollection = db.collection('captured_frames');
        const detectionCollection = db.collection('detection_results');
        
        // Get frame data
        const frame = await frameCollection.findOne({ _id: frameId });
        if (!frame) {
            return res.status(404).send('Frame not found');
        }
        
        // Check if detection overlay image exists
        const overlayPath = path.resolve(__dirname, '..', 'data', 'detections', `${frameId}_frame_detection_overlay.jpg`);
        
        if (fs.existsSync(overlayPath)) {
            // Serve pre-generated overlay image
            console.log(`✅ Serving detection overlay: ${frameId}`);
            res.sendFile(overlayPath);
            return;
        }
        
        // Check if there are any detections for this frame
        const detections = await detectionCollection.find({ frame_uuid: frameId }).toArray();
        if (detections.length === 0) {
            // No detections, return processed image (motion detection only)
            const imagePath = frame.processed_image_path || frame.original_image_path;
            if (imagePath) {
                const absolutePath = path.resolve(__dirname, '..', imagePath);
                if (fs.existsSync(absolutePath)) {
                    res.sendFile(absolutePath);
                    return;
                }
            }
        }
        
        // Generate overlay image using Python script
        const { spawn } = require('child_process');
        const pythonScript = path.resolve(__dirname, '..', 'src', 'image_detection', 'create_overlay.py');
        
        if (fs.existsSync(pythonScript)) {
            const pythonProcess = spawn('python', [pythonScript, frameId], {
                cwd: path.resolve(__dirname, '..'),
                stdio: 'pipe'
            });
            
            pythonProcess.on('close', (code) => {
                if (code === 0 && fs.existsSync(overlayPath)) {
                    res.sendFile(overlayPath);
                } else {
                    // Fallback to processed image
                    const imagePath = frame.processed_image_path || frame.original_image_path;
                    if (imagePath) {
                        const absolutePath = path.resolve(__dirname, '..', imagePath);
                        if (fs.existsSync(absolutePath)) {
                            res.sendFile(absolutePath);
                        } else {
                            res.status(404).send('Image not found');
                        }
                    } else {
                        res.status(404).send('No image path found');
                    }
                }
            });
        } else {
            // Fallback to processed image
            const imagePath = frame.processed_image_path || frame.original_image_path;
            if (imagePath) {
                const absolutePath = path.resolve(__dirname, '..', imagePath);
                if (fs.existsSync(absolutePath)) {
                    res.sendFile(absolutePath);
                } else {
                    res.status(404).send('Image not found');
                }
            } else {
                res.status(404).send('No image path found');
            }
        }
        
    } catch (error) {
        console.error('Error serving detection overlay:', error);
        res.status(500).send('Error serving detection overlay');
    }
});

// Fallback endpoint for missing images
app.get('/api/fallback/:frameId', (req, res) => {
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
        console.log(`🎯 Motion + Object Detection Viewer running on http://localhost:${PORT}`);
        console.log('📊 MongoDB: mongodb://localhost:27017/birds_of_play');
        console.log('🎯 Ready to view combined motion and object detection!');
    });
}

startServer();
