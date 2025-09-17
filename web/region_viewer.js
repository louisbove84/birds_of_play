#!/usr/bin/env node

const express = require('express');
const { MongoClient } = require('mongodb');
const path = require('path');
const fs = require('fs');

const app = express();
const PORT = 3001;

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

// Main page
app.get('/', (req, res) => {
    res.send(`
<!DOCTYPE html>
<html>
<head>
    <title>Birds of Play - Consolidated Regions Viewer</title>
    <style>
        body { 
            font-family: Arial, sans-serif; 
            margin: 20px; 
            background: #1a1a1a; 
            color: white; 
        }
        .container { max-width: 1200px; margin: 0 auto; }
        h1 { color: #9C27B0; text-align: center; }
        .nav-links {
            margin-top: 1rem;
            font-size: 0.9em;
            background: rgba(255, 255, 255, 0.1);
            padding: 0.5rem 1rem;
            border-radius: 8px;
            display: inline-block;
            text-align: center;
            width: 100%;
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
            color: #9C27B0;
            font-weight: bold;
            background: rgba(156, 39, 176, 0.1);
            padding: 0.5rem 1rem;
            border-radius: 4px;
            border: 1px solid rgba(156, 39, 176, 0.3);
        }
        .region-grid { 
            display: grid; 
            grid-template-columns: repeat(auto-fill, minmax(280px, 1fr)); 
            gap: 15px; 
            margin-top: 20px; 
        }
        .region-card { 
            background: #2a2a2a; 
            border-radius: 8px; 
            padding: 12px; 
            border: 1px solid #444; 
        }
        .region-card img { 
            width: 100%; 
            height: 150px; 
            object-fit: cover; 
            border-radius: 4px; 
            cursor: pointer;
            border: 2px solid transparent;
        }
        .region-card img:hover { border-color: #9C27B0; }
        .region-info { margin-top: 8px; font-size: 11px; color: #ccc; }
        .region-actions { margin-top: 8px; text-align: center; }
        .btn { 
            background: #9C27B0; 
            color: white; 
            border: none; 
            padding: 4px 8px; 
            border-radius: 4px; 
            cursor: pointer; 
            font-size: 10px;
            margin: 1px;
        }
        .btn:hover { background: #7B1FA2; }
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
        .stats { text-align: center; margin-bottom: 20px; color: #9C27B0; }
        .frame-ref {
            font-size: 10px;
            color: #888;
            background: #333;
            padding: 3px 6px;
            border-radius: 3px;
            margin-top: 5px;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🔍 Birds of Play - Consolidated Regions Viewer</h1>
        <p style="text-align: center; color: #ccc;">Individual consolidated motion regions extracted from frames - toggle to see motion detection context</p>
        
        <nav class="nav-links">
            <a href="http://localhost:3000" class="nav-link">
                📹 Motion Detection Frames
            </a>
            <span class="nav-separator">|</span>
            <span class="nav-current">🔍 Consolidated Regions</span>
        </nav>
        
        <div class="stats" id="stats">Loading...</div>
        <div id="regions" class="loading">Loading regions...</div>
    </div>

    <!-- Modal for full-size image -->
    <div id="modal" class="modal" onclick="closeModal()">
        <span class="close">&times;</span>
        <div class="modal-content">
            <img id="modal-img" src="" alt="Full size region">
        </div>
    </div>

    <script>
        async function loadRegions() {
            try {
                const response = await fetch('/api/regions');
                const data = await response.json();
                
                document.getElementById('stats').innerHTML = 
                    \`📊 Total Regions: \${data.regions.length} | 🎯 From \${data.frameCount} frames\`;
                
                const regionsContainer = document.getElementById('regions');
                regionsContainer.innerHTML = '';
                regionsContainer.className = 'region-grid';
                
                data.regions.forEach(region => {
                    const card = document.createElement('div');
                    card.className = 'region-card';
                    
                    const timestamp = new Date(region.timestamp).toLocaleString();
                    
                    card.innerHTML = \`
                        <img id="img-\${region.id}" src="/api/region-image/\${region.id}" 
                             alt="Region \${region.id}" 
                             onclick="openModal('/api/region-image/\${region.id}')"
                             onerror="this.src='/api/fallback/\${region.id}'">
                        <div class="region-info">
                            <strong>Region:</strong> \${region.regionIndex + 1} of \${region.totalRegions}<br>
                            <strong>Size:</strong> \${region.width}×\${region.height}px<br>
                            <strong>Position:</strong> (\${region.x}, \${region.y})<br>
                            <strong>Time:</strong> \${timestamp}
                        </div>
                        <div class="frame-ref">
                            Frame: \${region.frameId.substring(0, 8)}...
                        </div>
                        <div class="region-actions">
                            <button class="btn" onclick="toggleImage('\${region.id}', 'region')">🔍 Region</button>
                            <button class="btn secondary" onclick="toggleImage('\${region.id}', 'frame')">📷 Motion Frame</button>
                        </div>
                    \`;
                    
                    regionsContainer.appendChild(card);
                });
                
            } catch (error) {
                console.error('Error loading regions:', error);
                document.getElementById('regions').innerHTML = 
                    '<div style="color: red; text-align: center;">Error loading regions. Check console for details.</div>';
            }
        }
        
        function toggleImage(regionId, type) {
            const img = document.getElementById(\`img-\${regionId}\`);
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
            if (type === 'frame') {
                img.src = \`/api/frame-thumbnail/\${regionId}\`;
            } else {
                img.src = \`/api/region-image/\${regionId}\`;
            }
        }
        
        function openModal(imageSrc) {
            document.getElementById('modal-img').src = imageSrc;
            document.getElementById('modal').style.display = 'block';
        }
        
        function closeModal() {
            document.getElementById('modal').style.display = 'none';
        }
        
        // Load regions when page loads
        window.addEventListener('load', loadRegions);
        
        // Close modal with Escape key
        document.addEventListener('keydown', function(e) {
            if (e.key === 'Escape') closeModal();
        });
    </script>
</body>
</html>
    `);
});

// API endpoint to get all consolidated regions from all frames
app.get('/api/regions', async (req, res) => {
    try {
        console.log('🔍 Fetching frames from MongoDB...');
        const collection = db.collection('captured_frames');
        const frames = await collection.find({})
            .sort({ timestamp: 1 }) // FIFO order
            .toArray();
        
        console.log(`📊 Found ${frames.length} frames in MongoDB`);
        
        const regions = [];
        let regionIdCounter = 0;
        
        frames.forEach((frame, frameIndex) => {
            const consolidatedRegions = frame.metadata?.consolidated_regions || [];
            console.log(`Frame ${frameIndex + 1}: ${frame._id} has ${consolidatedRegions.length} consolidated regions`);
            
            consolidatedRegions.forEach((region, index) => {
                console.log(`  Region ${index}: ${region.width}x${region.height} at (${region.x}, ${region.y})`);
                regions.push({
                    id: `${frame._id}_${index}`,
                    frameId: frame._id,
                    regionIndex: index,
                    totalRegions: consolidatedRegions.length,
                    x: region.x,
                    y: region.y,
                    width: region.width,
                    height: region.height,
                    timestamp: frame.timestamp
                });
            });
        });
        
        console.log(`✅ Total consolidated regions extracted: ${regions.length}`);
        
        res.json({ 
            regions: regions,
            frameCount: frames.length
        });
    } catch (error) {
        console.error('❌ Error fetching regions:', error);
        res.status(500).json({ error: 'Failed to fetch regions' });
    }
});

// API endpoint to serve region images (cropped from original frame)
app.get('/api/region-image/:regionId', async (req, res) => {
    try {
        const { regionId } = req.params;
        const [frameId, regionIndex] = regionId.split('_');
        
        const collection = db.collection('captured_frames');
        const frame = await collection.findOne({ _id: frameId });
        
        if (!frame) {
            return res.status(404).send('Frame not found');
        }
        
        const regions = frame.metadata?.consolidated_regions || [];
        const region = regions[parseInt(regionIndex)];
        
        if (!region) {
            return res.status(404).send('Region not found');
        }
        
        // Check if cropped region image already exists
        const regionImagePath = path.resolve(__dirname, '..', 'data', 'regions', `${regionId}.jpg`);
        
        if (fs.existsSync(regionImagePath)) {
            res.sendFile(regionImagePath);
            return;
        }
        
        // Generate region image using Python script
        const { spawn } = require('child_process');
        const pythonScript = path.resolve(__dirname, 'extract_region.py');
        
        console.log(`🔍 Attempting to extract region ${regionId}`);
        console.log(`📄 Python script: ${pythonScript}`);
        console.log(`📁 Output path: ${regionImagePath}`);
        
        if (fs.existsSync(pythonScript)) {
            // Use the virtual environment Python
            const venvPython = path.resolve(__dirname, '..', 'venv', 'bin', 'python');
            let pythonCmd = fs.existsSync(venvPython) ? venvPython : 'python3';
            
            console.log(`🐍 Using Python: ${pythonCmd}`);
            
            const pythonProcess = spawn(pythonCmd, [pythonScript, frameId, regionIndex.toString()], {
                cwd: __dirname,
                stdio: 'pipe'
            });
            
            pythonProcess.stdout.on('data', (data) => {
                console.log(`Python stdout: ${data}`);
            });
            
            pythonProcess.stderr.on('data', (data) => {
                console.error(`Python stderr: ${data}`);
            });
            
            pythonProcess.on('error', (error) => {
                console.error(`❌ Python process error: ${error.message}`);
                res.status(500).send(`Python execution error: ${error.message}`);
            });
            
            pythonProcess.on('close', (code) => {
                console.log(`Python process closed with code: ${code}`);
                if (code === 0 && fs.existsSync(regionImagePath)) {
                    console.log(`✅ Region extracted successfully: ${regionImagePath}`);
                    res.sendFile(regionImagePath);
                } else {
                    console.log(`❌ Region extraction failed. Code: ${code}, File exists: ${fs.existsSync(regionImagePath)}`);
                    res.status(404).send(`Could not extract region (exit code: ${code})`);
                }
            });
        } else {
            console.log(`❌ Python script not found: ${pythonScript}`);
            res.status(404).send('Region extraction script not available');
        }
        
    } catch (error) {
        console.error('Error serving region image:', error);
        res.status(500).send('Error serving region image');
    }
});

// API endpoint to serve frame thumbnail (with motion detection boxes)
app.get('/api/frame-thumbnail/:regionId', async (req, res) => {
    try {
        const { regionId } = req.params;
        const [frameId] = regionId.split('_');
        
        const collection = db.collection('captured_frames');
        const frame = await collection.findOne({ _id: frameId });
        
        if (!frame) {
            return res.status(404).send('Frame not found');
        }
        
        // Use processed image path (with motion detection boxes) instead of original
        const imagePath = frame.processed_image_path || frame.original_image_path;
        if (!imagePath) {
            return res.status(404).send('No processed image path found');
        }
        
        const absolutePath = path.resolve(__dirname, '..', imagePath);
        
        if (!fs.existsSync(absolutePath)) {
            console.log(`❌ Processed image not found: ${absolutePath}, trying original...`);
            
            // Fallback to original if processed doesn't exist
            const originalPath = frame.original_image_path;
            if (originalPath) {
                const originalAbsolutePath = path.resolve(__dirname, '..', originalPath);
                if (fs.existsSync(originalAbsolutePath)) {
                    res.sendFile(originalAbsolutePath);
                    return;
                }
            }
            
            return res.status(404).send('Motion detection image file not found');
        }
        
        console.log(`✅ Serving motion detection frame: ${frameId}`);
        res.sendFile(absolutePath);
        
    } catch (error) {
        console.error('Error serving frame thumbnail:', error);
        res.status(500).send('Error serving frame thumbnail');
    }
});

// Fallback endpoint for missing images
app.get('/api/fallback/:regionId', (req, res) => {
    res.send(`
        <svg width="280" height="150" xmlns="http://www.w3.org/2000/svg">
            <rect width="100%" height="100%" fill="#333"/>
            <text x="50%" y="50%" text-anchor="middle" fill="white" font-family="Arial" font-size="12">
                Region not found
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
        console.log(`🔍 Consolidated Regions Viewer running on http://localhost:${PORT}`);
        console.log('📊 MongoDB: mongodb://localhost:27017/birds_of_play');
        console.log('🎯 Ready to view individual consolidated regions!');
    });
}

startServer();
