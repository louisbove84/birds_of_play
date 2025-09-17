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
        .detection-results {
            margin-top: 8px;
            padding: 8px;
            background: #333;
            border-radius: 4px;
            font-size: 11px;
        }
        .detection-loading {
            color: #FF6B35;
            text-align: center;
            font-style: italic;
        }
        .detection-item {
            background: #444;
            padding: 4px 6px;
            margin: 2px 0;
            border-radius: 3px;
            border-left: 3px solid #4CAF50;
        }
        .detection-confidence {
            color: #4CAF50;
            font-weight: bold;
        }
        .no-detections {
            color: #888;
            text-align: center;
            font-style: italic;
        }
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
        <h1>🎯 Birds of Play - Object Detection Viewer</h1>
        <p style="text-align: center; color: #ccc;">Individual detected objects cropped from consolidated motion regions</p>
        
        <nav class="nav-links">
            <a href="http://localhost:3000" class="nav-link" target="_self">
                📹 Motion Detection Frames
            </a>
            <span class="nav-separator">|</span>
            <span class="nav-current">🎯 Object Detections</span>
            <span class="nav-separator">|</span>
            <a href="http://localhost:3002/dashboard" class="nav-link" target="_self">
                🔬 Bird Clustering
            </a>
        </nav>
        
        <div class="controls" style="background: #2a2a2a; padding: 15px; border-radius: 8px; margin: 20px 0; text-align: center;">
            <button class="btn" onclick="runBatchDetection()" id="batchBtn">🚀 Run YOLO11 on All Regions</button>
            <button class="btn secondary" onclick="loadRegions()" id="refreshBtn">🔄 Refresh</button>
            <button class="btn secondary" onclick="loadHighConfidenceDetections()" id="detectionBtn">🎯 View High-Confidence Detections</button>
        </div>
        
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
                const response = await fetch('/api/objects');
                const data = await response.json();
                
                document.getElementById('stats').innerHTML = 
                    \`🔥 High-Confidence Objects: \${data.objects.length} | 🎯 From \${data.regionCount} regions (≥93%)\`;
                
                const regionsContainer = document.getElementById('regions');
                regionsContainer.innerHTML = '';
                regionsContainer.className = 'region-grid';
                
                data.objects.forEach(object => {
                    const card = document.createElement('div');
                    card.className = 'region-card';
                    
                    const timestamp = new Date(object.timestamp).toLocaleString();
                    const confidence = (object.confidence * 100).toFixed(1);
                    
                    card.innerHTML = \`
                        <img id="img-\${object.id}" src="/api/object-image/\${object.id}" 
                             alt="Object \${object.id}" 
                             onclick="openModal('/api/object-image/\${object.id}')"
                             onerror="this.src='/api/fallback/\${object.id}'">
                        <div class="region-info">
                            <strong>🔥 \${object.displayName}:</strong> \${confidence}%<br>
                            <strong>Region:</strong> \${object.regionIndex + 1}<br>
                            <strong>Position:</strong> [\${object.bbox[0]}, \${object.bbox[1]}, \${object.bbox[2]}, \${object.bbox[3]}]<br>
                            <strong>Time:</strong> \${timestamp}
                        </div>
                        <div class="frame-ref">
                            Frame: \${object.frameId.substring(0, 8)}...
                        </div>
                        <div class="region-actions">
                            <button class="btn" onclick="toggleImage('\${object.id}', 'object')">🎯 Object</button>
                            <button class="btn secondary" onclick="toggleImage('\${object.id}', 'frame')">📷 Motion Frame</button>
                        </div>
                        <div class="detection-results" style="display: block;">
                            <div class="detection-item">
                                🔥 \${object.displayName} - <span class="detection-confidence">\${confidence}%</span>
                            </div>
                        </div>
                    \`;
                    
                    regionsContainer.appendChild(card);
                });
                
            } catch (error) {
                console.error('Error loading objects:', error);
                document.getElementById('regions').innerHTML = 
                    '<div style="color: red; text-align: center;">Error loading objects. Check console for details.</div>';
            }
        }
        
        function toggleImage(objectId, type) {
            const img = document.getElementById(\`img-\${objectId}\`);
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
                img.src = \`/api/frame-thumbnail/\${objectId}\`;  // Pass objectId for highlighting
            } else {
                // Default to object view
                img.src = \`/api/object-image/\${objectId}\`;
            }
        }
        
        async function runBatchDetection() {
            const batchBtn = document.getElementById('batchBtn');
            const statsDiv = document.getElementById('stats');
            
            // Show loading state
            batchBtn.disabled = true;
            batchBtn.textContent = '⏳ Running YOLO11...';
            statsDiv.innerHTML = '🔍 Running YOLO11 detection on all regions...';
            
            try {
                const response = await fetch('/api/batch-detection', { method: 'POST' });
                const result = await response.json();
                
                if (result.success) {
                    statsDiv.innerHTML = \`✅ Detection completed! \${result.processed} regions processed, \${result.highConfidence} high-confidence detections\`;
                    await loadRegions(); // Refresh the view
                } else {
                    statsDiv.innerHTML = \`❌ Batch detection failed: \${result.error}\`;
                }
            } catch (error) {
                console.error('Batch detection error:', error);
                statsDiv.innerHTML = '❌ Batch detection failed. Check console for details.';
            } finally {
                batchBtn.disabled = false;
                batchBtn.textContent = '🚀 Run YOLO11 on All Regions';
            }
        }
        
        
        async function loadHighConfidenceDetections() {
            try {
                const response = await fetch('/api/high-confidence-detections');
                const data = await response.json();
                
                document.getElementById('stats').innerHTML = 
                    \`🔥 High-Confidence Detections: \${data.detections.length} (>90% confidence)\`;
                
                const regionsContainer = document.getElementById('regions');
                regionsContainer.innerHTML = '';
                regionsContainer.className = 'region-grid';
                
                if (data.detections.length === 0) {
                    regionsContainer.innerHTML = '<div style="text-align: center; color: #888; padding: 50px;">No high-confidence detections found. Run batch detection first.</div>';
                    return;
                }
                
                data.detections.forEach(detection => {
                    const card = document.createElement('div');
                    card.className = 'region-card';
                    
                    const timestamp = new Date(detection.timestamp).toLocaleString();
                    const confidence = (detection.detection_info.confidence * 100).toFixed(1);
                    
                    card.innerHTML = \`
                        <img src="/api/detection-image/\${detection.detection_id}" 
                             alt="Detection \${detection.detection_id}" 
                             onclick="openModal('/api/detection-image/\${detection.detection_id}')"
                             onerror="this.src='/api/fallback/\${detection.detection_id}'">
                        <div class="region-info">
                            <strong>Object:</strong> \${detection.detection_info.class_name}<br>
                            <strong>Confidence:</strong> <span class="detection-confidence">\${confidence}%</span><br>
                            <strong>Region:</strong> \${detection.region_index + 1}<br>
                            <strong>Time:</strong> \${timestamp}
                        </div>
                        <div class="frame-ref">
                            Frame: \${detection.frame_id.substring(0, 8)}...
                        </div>
                    \`;
                    
                    regionsContainer.appendChild(card);
                });
                
            } catch (error) {
                console.error('Error loading high-confidence detections:', error);
                document.getElementById('regions').innerHTML = 
                    '<div style="color: red; text-align: center;">Error loading high-confidence detections. Check console for details.</div>';
            }
        }
        
        async function loadDetectionResults(regionId) {
            try {
                const response = await fetch(\`/api/detection-results/\${regionId}\`);
                const results = await response.json();
                
                const resultsDiv = document.getElementById(\`detection-results-\${regionId}\`);
                
                if (results.detections && results.detections.length > 0) {
                    let html = \`<strong>🎯 Objects Found (\${results.detections.length}):</strong><br>\`;
                    results.detections.forEach(detection => {
                        const confidence = (detection.confidence * 100).toFixed(1);
                        html += \`<div class="detection-item">
                            \${detection.class_name} - <span class="detection-confidence">\${confidence}%</span>
                        </div>\`;
                    });
                    resultsDiv.innerHTML = html;
                } else {
                    resultsDiv.innerHTML = '<div class="no-detections">No objects detected</div>';
                }
            } catch (error) {
                console.error('Error loading detection results:', error);
                const resultsDiv = document.getElementById(\`detection-results-\${regionId}\`);
                resultsDiv.innerHTML = '<div class="no-detections">Error loading results</div>';
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

// API endpoint to get individual object detections
app.get('/api/objects', async (req, res) => {
    try {
        console.log('🔍 Fetching individual object detections...');
        const detectionCollection = db.collection('region_detections');
        
        // Get all regions that have detections
        const regionsWithDetections = await detectionCollection.find({
            detection_count: { $gt: 0 }
        }).sort({ timestamp: 1 }).toArray();
        
        console.log(`📊 Found ${regionsWithDetections.length} regions with detections`);
        
        const objects = [];
        
        regionsWithDetections.forEach(regionData => {
            const regionId = regionData.region_id;
            const [frameId, regionIndex] = regionId.split('_');
            
            // Create individual object cards for each detection
            regionData.detections.forEach((detection, detectionIndex) => {
                const objectId = `${regionId}_obj_${detectionIndex}`;
                
                objects.push({
                    id: objectId,
                    regionId: regionId,
                    frameId: frameId,
                    regionIndex: parseInt(regionIndex),
                    detectionIndex: detectionIndex,
                    objectClass: detection.class_name,
                    displayName: detection.display_name || detection.class_name,
                    confidence: detection.confidence,
                    bbox: detection.bbox,
                    regionMetadata: regionData.region_metadata,
                    timestamp: regionData.timestamp
                });
            });
        });
        
        console.log(`✅ Returning ${objects.length} individual object detections`);
        
        res.json({ 
            objects: objects,
            regionCount: regionsWithDetections.length
        });
    } catch (error) {
        console.error('❌ Error fetching object detections:', error);
        res.status(500).json({ error: 'Failed to fetch object detections' });
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
        const pythonScript = path.resolve(__dirname, '..', 'src', 'image_detection', 'extract_region.py');
        
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

// API endpoint to serve frame thumbnail with highlighted region
app.get('/api/frame-thumbnail/:objectId', async (req, res) => {
    try {
        const { objectId } = req.params;
        
        // Check if highlighted frame already exists
        const highlightedPath = path.resolve(__dirname, '..', 'data', 'highlighted_frames', `${objectId}_highlighted_frame.jpg`);
        
        if (fs.existsSync(highlightedPath)) {
            console.log(`✅ Serving highlighted frame: ${objectId}`);
            res.sendFile(highlightedPath);
            return;
        }
        
        // Generate highlighted frame using Python script
        const { spawn } = require('child_process');
        const venvPython = path.resolve(__dirname, '..', 'venv', 'bin', 'python');
        const pythonCmd = fs.existsSync(venvPython) ? venvPython : 'python3';
        const scriptPath = path.resolve(__dirname, '..', 'src', 'image_detection', 'highlight_region_in_frame.py');
        
        console.log(`🔍 Generating highlighted frame for ${objectId}`);
        
        if (!fs.existsSync(scriptPath)) {
            // Fallback to regular processed frame
            const regionId = objectId.split('_obj_')[0];
            const [frameId] = regionId.split('_');
            
            const collection = db.collection('captured_frames');
            const frame = await collection.findOne({ _id: frameId });
            
            if (frame && frame.processed_image_path) {
                const absolutePath = path.resolve(__dirname, '..', frame.processed_image_path);
                if (fs.existsSync(absolutePath)) {
                    res.sendFile(absolutePath);
                    return;
                }
            }
            
            return res.status(404).send('Highlight script not found and no fallback available');
        }
        
        const pythonProcess = spawn(pythonCmd, [scriptPath, objectId], {
            cwd: __dirname,
            stdio: 'pipe'
        });
        
        pythonProcess.stdout.on('data', (data) => {
            console.log(`Highlight generation: ${data}`);
        });
        
        pythonProcess.stderr.on('data', (data) => {
            console.error(`Highlight generation error: ${data}`);
        });
        
        pythonProcess.on('close', (code) => {
            if (code === 0 && fs.existsSync(highlightedPath)) {
                console.log(`✅ Highlighted frame generated: ${objectId}`);
                res.sendFile(highlightedPath);
            } else {
                console.log(`❌ Highlight generation failed: ${objectId}`);
                
                // Fallback to regular processed frame
                const regionId = objectId.split('_obj_')[0];
                const [frameId] = regionId.split('_');
                
                db.collection('captured_frames').findOne({ _id: frameId }).then(frame => {
                    if (frame && frame.processed_image_path) {
                        const absolutePath = path.resolve(__dirname, '..', frame.processed_image_path);
                        if (fs.existsSync(absolutePath)) {
                            res.sendFile(absolutePath);
                        } else {
                            res.status(404).send('Could not generate highlighted frame');
                        }
                    } else {
                        res.status(404).send('Could not generate highlighted frame');
                    }
                });
            }
        });
        
        pythonProcess.on('error', (error) => {
            console.error(`Highlight process error: ${error}`);
            res.status(500).send(`Highlight generation error: ${error.message}`);
        });
        
    } catch (error) {
        console.error('Error serving highlighted frame:', error);
        res.status(500).send('Error serving highlighted frame');
    }
});

// API endpoint to run batch detection on all regions
app.post('/api/batch-detection', async (req, res) => {
    try {
        console.log(`🚀 Starting batch detection on all regions`);
        
        // Run the batch detection Python script
        const { spawn } = require('child_process');
        const venvPython = path.resolve(__dirname, '..', 'venv', 'bin', 'python');
        const pythonCmd = fs.existsSync(venvPython) ? venvPython : 'python3';
        const scriptPath = path.resolve(__dirname, '..', 'src', 'image_detection', 'batch_detect_regions.py');
        
        if (!fs.existsSync(scriptPath)) {
            return res.status(500).json({ success: false, error: 'Batch detection script not found' });
        }
        
        const pythonProcess = spawn(pythonCmd, [scriptPath], {  // Uses config file for thresholds
            cwd: __dirname,
            stdio: 'pipe'
        });
        
        let stdout = '';
        let stderr = '';
        
        pythonProcess.stdout.on('data', (data) => {
            stdout += data.toString();
            console.log(`Batch detection: ${data}`);
        });
        
        pythonProcess.stderr.on('data', (data) => {
            stderr += data.toString();
            console.error(`Batch detection stderr: ${data}`);
        });
        
        pythonProcess.on('close', (code) => {
            if (code === 0) {
                console.log(`✅ Batch detection completed successfully`);
                
                // Parse results from stdout to get counts
                const processedMatch = stdout.match(/Processed (\d+)\/\d+ regions/);
                const highConfMatch = stdout.match(/Found (\d+) high-confidence detections/);
                
                const processed = processedMatch ? parseInt(processedMatch[1]) : 0;
                const highConfidence = highConfMatch ? parseInt(highConfMatch[1]) : 0;
                
                res.json({ 
                    success: true, 
                    message: 'Batch detection completed',
                    processed: processed,
                    highConfidence: highConfidence
                });
            } else {
                console.log(`❌ Batch detection failed with code ${code}`);
                res.status(500).json({ success: false, error: `Batch detection failed with exit code ${code}` });
            }
        });
        
        pythonProcess.on('error', (error) => {
            console.error(`Batch detection process error: ${error}`);
            res.status(500).json({ success: false, error: error.message });
        });
        
    } catch (error) {
        console.error('Error running batch detection:', error);
        res.status(500).json({ success: false, error: error.message });
    }
});

// API endpoint to get detection results for a region
app.get('/api/detection-results/:regionId', async (req, res) => {
    try {
        const { regionId } = req.params;
        
        // First try to get from MongoDB
        const collection = db.collection('region_detections');
        const mongoResults = await collection.findOne({ region_id: regionId });
        
        if (mongoResults) {
            res.json(mongoResults);
            return;
        }
        
        // Fallback to JSON file
        const resultsPath = path.resolve(__dirname, '..', 'data', 'regions', `${regionId}_results.json`);
        
        if (fs.existsSync(resultsPath)) {
            const fileContent = fs.readFileSync(resultsPath, 'utf8');
            const results = JSON.parse(fileContent);
            res.json(results);
        } else {
            res.status(404).json({ error: 'Detection results not found' });
        }
        
    } catch (error) {
        console.error('Error getting detection results:', error);
        res.status(500).json({ error: error.message });
    }
});

// API endpoint to serve detection overlay images
app.get('/api/detection-overlay/:regionId', async (req, res) => {
    try {
        const { regionId } = req.params;
        
        // Check for detection overlay image
        const overlayPath = path.resolve(__dirname, '..', 'data', 'regions', `${regionId}_detection.jpg`);
        
        if (fs.existsSync(overlayPath)) {
            console.log(`✅ Serving detection overlay: ${regionId}`);
            res.sendFile(overlayPath);
        } else {
            // Fallback to original region image
            const originalPath = path.resolve(__dirname, '..', 'data', 'regions', `${regionId}.jpg`);
            if (fs.existsSync(originalPath)) {
                res.sendFile(originalPath);
            } else {
                res.status(404).send('Detection overlay not found');
            }
        }
        
    } catch (error) {
        console.error('Error serving detection overlay:', error);
        res.status(500).send('Error serving detection overlay');
    }
});

// API endpoint to get high-confidence detections
app.get('/api/high-confidence-detections', async (req, res) => {
    try {
        const collection = db.collection('high_confidence_detections');
        const detections = await collection.find({})
            .sort({ timestamp: 1 })  // FIFO order
            .toArray();
            
        res.json({ detections });
    } catch (error) {
        console.error('Error fetching high-confidence detections:', error);
        res.status(500).json({ error: 'Failed to fetch high-confidence detections' });
    }
});

// API endpoint to serve detection images for individual high-confidence detections
app.get('/api/detection-image/:detectionId', async (req, res) => {
    try {
        const { detectionId } = req.params;
        
        // Parse detection ID to get region ID
        const regionId = detectionId.split('_det_')[0];
        
        // Check for detection overlay image
        const overlayPath = path.resolve(__dirname, '..', 'data', 'regions', `${regionId}_detection.jpg`);
        
        if (fs.existsSync(overlayPath)) {
            console.log(`✅ Serving detection image: ${detectionId}`);
            res.sendFile(overlayPath);
        } else {
            // Fallback to original region image
            const originalPath = path.resolve(__dirname, '..', 'data', 'regions', `${regionId}.jpg`);
            if (fs.existsSync(originalPath)) {
                res.sendFile(originalPath);
            } else {
                res.status(404).send('Detection image not found');
            }
        }
        
    } catch (error) {
        console.error('Error serving detection image:', error);
        res.status(500).send('Error serving detection image');
    }
});

// API endpoint to serve individual object images (cropped to bounding box)
app.get('/api/object-image/:objectId', async (req, res) => {
    try {
        const { objectId } = req.params;
        
        // Parse object ID: regionId_obj_detectionIndex
        const parts = objectId.split('_obj_');
        if (parts.length !== 2) {
            return res.status(400).send('Invalid object ID format');
        }
        
        const regionId = parts[0];
        const detectionIndex = parseInt(parts[1]);
        
        console.log(`🔍 Extracting object ${objectId} (region: ${regionId}, detection: ${detectionIndex})`);
        
        // Check if object image already exists
        const objectImagePath = path.resolve(__dirname, '..', 'data', 'objects', `${objectId}.jpg`);
        
        if (fs.existsSync(objectImagePath)) {
            console.log(`✅ Serving cached object image: ${objectId}`);
            res.sendFile(objectImagePath);
            return;
        }
        
        // Generate object image using Python script
        const { spawn } = require('child_process');
        const venvPython = path.resolve(__dirname, '..', 'venv', 'bin', 'python');
        const pythonCmd = fs.existsSync(venvPython) ? venvPython : 'python3';
        const scriptPath = path.resolve(__dirname, '..', 'src', 'image_detection', 'extract_object.py');
        
        if (!fs.existsSync(scriptPath)) {
            return res.status(500).send('Object extraction script not found');
        }
        
        const pythonProcess = spawn(pythonCmd, [scriptPath, objectId], {
            cwd: __dirname,
            stdio: 'pipe'
        });
        
        pythonProcess.stdout.on('data', (data) => {
            console.log(`Object extraction: ${data}`);
        });
        
        pythonProcess.stderr.on('data', (data) => {
            console.error(`Object extraction error: ${data}`);
        });
        
        pythonProcess.on('close', (code) => {
            if (code === 0 && fs.existsSync(objectImagePath)) {
                console.log(`✅ Object extracted successfully: ${objectId}`);
                res.sendFile(objectImagePath);
            } else {
                console.log(`❌ Object extraction failed: ${objectId}`);
                res.status(404).send('Could not extract object');
            }
        });
        
        pythonProcess.on('error', (error) => {
            console.error(`Object extraction process error: ${error}`);
            res.status(500).send(`Object extraction error: ${error.message}`);
        });
        
    } catch (error) {
        console.error('Error serving object image:', error);
        res.status(500).send('Error serving object image');
    }
});

// Fallback endpoint for missing images
app.get('/api/fallback/:objectId', (req, res) => {
    res.send(`
        <svg width="280" height="150" xmlns="http://www.w3.org/2000/svg">
            <rect width="100%" height="100%" fill="#333"/>
            <text x="50%" y="50%" text-anchor="middle" fill="white" font-family="Arial" font-size="12">
                Object not found
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
