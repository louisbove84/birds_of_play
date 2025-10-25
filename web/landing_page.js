const express = require('express');
const multer = require('multer');
const path = require('path');
const { spawn } = require('child_process');
const fs = require('fs');

const app = express();
const PORT = 3004;

// Configure multer for video uploads
const storage = multer.diskStorage({
    destination: function (req, file, cb) {
        const uploadDir = path.join(__dirname, '..', 'uploads');
        if (!fs.existsSync(uploadDir)) {
            fs.mkdirSync(uploadDir, { recursive: true });
        }
        cb(null, uploadDir);
    },
    filename: function (req, file, cb) {
        // Generate unique filename with timestamp
        const timestamp = Date.now();
        const ext = path.extname(file.originalname);
        cb(null, `user_video_${timestamp}${ext}`);
    }
});

const upload = multer({
    storage: storage,
    fileFilter: function (req, file, cb) {
        // Accept video files
        const allowedTypes = /mp4|avi|mov|mkv|wmv|flv|webm/;
        const extname = allowedTypes.test(path.extname(file.originalname).toLowerCase());
        const mimetype = file.mimetype.startsWith('video/');
        
        if (mimetype && extname) {
            return cb(null, true);
        } else {
            cb(new Error('Only video files are allowed!'));
        }
    },
    limits: {
        fileSize: 500 * 1024 * 1024 // 500MB limit
    }
});

// Serve static files
app.use(express.static(path.join(__dirname, 'public')));
app.use(express.json());

// Store active pipeline processes
const activePipelines = new Map();

// Landing page
app.get('/', (req, res) => {
    res.send(`
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>🦅 Birds of Play - Video Analysis Pipeline</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #1e3c72 0%, #2a5298 100%);
            color: white;
            min-height: 100vh;
            display: flex;
            align-items: center;
            justify-content: center;
        }
        
        .container {
            max-width: 800px;
            padding: 40px;
            text-align: center;
            background: rgba(255, 255, 255, 0.1);
            border-radius: 20px;
            backdrop-filter: blur(10px);
            box-shadow: 0 8px 32px rgba(0, 0, 0, 0.3);
        }
        
        .logo {
            font-size: 4rem;
            margin-bottom: 20px;
            animation: float 3s ease-in-out infinite;
        }
        
        @keyframes float {
            0%, 100% { transform: translateY(0px); }
            50% { transform: translateY(-10px); }
        }
        
        h1 {
            font-size: 3rem;
            margin-bottom: 10px;
            background: linear-gradient(45deg, #FF6B35, #F7931E);
            -webkit-background-clip: text;
            -webkit-text-fill-color: transparent;
            background-clip: text;
        }
        
        .subtitle {
            font-size: 1.2rem;
            margin-bottom: 40px;
            opacity: 0.9;
            line-height: 1.6;
        }
        
        .options-container {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(280px, 1fr));
            gap: 20px;
            margin: 40px 0;
            max-width: 1200px;
            margin-left: auto;
            margin-right: auto;
        }
        
        .option-card {
            background: rgba(255, 255, 255, 0.1);
            border-radius: 15px;
            padding: 30px;
            border: 2px solid transparent;
            transition: all 0.3s ease;
            cursor: pointer;
            position: relative;
            overflow: hidden;
        }
        
        .option-card:hover {
            transform: translateY(-5px);
            border-color: #FF6B35;
            background: rgba(255, 255, 255, 0.15);
        }
        
        .option-card::before {
            content: '';
            position: absolute;
            top: 0;
            left: -100%;
            width: 100%;
            height: 100%;
            background: linear-gradient(90deg, transparent, rgba(255, 255, 255, 0.1), transparent);
            transition: left 0.5s;
        }
        
        .option-card:hover::before {
            left: 100%;
        }
        
        .option-icon {
            font-size: 3rem;
            margin-bottom: 20px;
            display: block;
        }
        
        .option-title {
            font-size: 1.5rem;
            font-weight: bold;
            margin-bottom: 15px;
            color: #FF6B35;
        }
        
        .option-description {
            font-size: 1rem;
            line-height: 1.6;
            opacity: 0.9;
            margin-bottom: 20px;
        }
        
        .upload-area {
            border: 2px dashed #FF6B35;
            border-radius: 10px;
            padding: 20px;
            margin: 20px 0;
            transition: all 0.3s ease;
            background: rgba(255, 107, 53, 0.1);
        }
        
        .upload-area:hover {
            background: rgba(255, 107, 53, 0.2);
        }
        
        .upload-area.dragover {
            border-color: #F7931E;
            background: rgba(247, 147, 30, 0.2);
        }
        
        .file-input {
            display: none;
        }
        
        .upload-button {
            background: linear-gradient(45deg, #FF6B35, #F7931E);
            color: white;
            border: none;
            padding: 12px 30px;
            border-radius: 25px;
            font-size: 1rem;
            font-weight: bold;
            cursor: pointer;
            transition: all 0.3s ease;
            margin: 10px;
        }
        
        .upload-button:hover {
            transform: scale(1.05);
            box-shadow: 0 5px 15px rgba(255, 107, 53, 0.4);
        }
        
        .preload-button {
            background: linear-gradient(45deg, #4CAF50, #45a049);
            color: white;
            border: none;
            padding: 15px 40px;
            border-radius: 25px;
            font-size: 1.1rem;
            font-weight: bold;
            cursor: pointer;
            transition: all 0.3s ease;
            margin: 10px;
        }
        
        .preload-button:hover {
            transform: scale(1.05);
            box-shadow: 0 5px 15px rgba(76, 175, 80, 0.4);
        }
        
        .status-container {
            margin-top: 30px;
            padding: 20px;
            background: rgba(0, 0, 0, 0.3);
            border-radius: 10px;
            display: none;
        }
        
        .status-container.active {
            display: block;
        }
        
        .progress-bar {
            width: 100%;
            height: 20px;
            background: rgba(255, 255, 255, 0.2);
            border-radius: 10px;
            overflow: hidden;
            margin: 15px 0;
        }
        
        .progress-fill {
            height: 100%;
            background: linear-gradient(45deg, #FF6B35, #F7931E);
            width: 0%;
            transition: width 0.3s ease;
        }
        
        .status-text {
            font-size: 1rem;
            margin: 10px 0;
        }
        
        .success-message {
            color: #4CAF50;
            font-weight: bold;
        }
        
        .error-message {
            color: #f44336;
            font-weight: bold;
        }
        
        .nav-links {
            position: fixed;
            top: 20px;
            right: 20px;
            display: flex;
            gap: 10px;
        }
        
        .nav-link {
            background: rgba(255, 255, 255, 0.1);
            color: white;
            padding: 10px 15px;
            border-radius: 20px;
            text-decoration: none;
            font-size: 0.9rem;
            transition: all 0.3s ease;
        }
        
        .nav-link:hover {
            background: rgba(255, 255, 255, 0.2);
            transform: translateY(-2px);
        }
        
        .file-info {
            margin: 10px 0;
            padding: 10px;
            background: rgba(255, 255, 255, 0.1);
            border-radius: 5px;
            font-size: 0.9rem;
        }
        
        @media (max-width: 768px) {
            .container {
                margin: 20px;
                padding: 20px;
            }
            
            h1 {
                font-size: 2rem;
            }
            
            .options-container {
                grid-template-columns: 1fr;
            }
            
            .nav-links {
                position: static;
                justify-content: center;
                margin-bottom: 20px;
            }
        }
    </style>
</head>
<body>
    <nav class="nav-links">
        <a href="http://localhost:3000" class="nav-link" target="_blank">📹 Motion</a>
        <a href="http://localhost:3001" class="nav-link" target="_blank">🎯 Objects</a>
        <a href="http://localhost:3002" class="nav-link" target="_blank">🔬 Clustering</a>
        <a href="http://localhost:3003" class="nav-link" target="_blank">🧠 Fine-Tuning</a>
    </nav>

    <div class="container">
        <div class="logo">🦅</div>
        <h1>Birds of Play</h1>
        <p class="subtitle">
            Advanced AI-powered bird detection and classification pipeline.<br>
            Upload your video or use our sample to discover bird species through motion detection, 
            object recognition, clustering, and machine learning.
        </p>
        
        <div class="options-container">
            <!-- Upload Video Option -->
            <div class="option-card" id="upload-card">
                <div class="option-icon">📁</div>
                <div class="option-title">Upload Your Video</div>
                <div class="option-description">
                    Upload your own bird video for analysis. Supports MP4, AVI, MOV, MKV, and other common video formats.
                </div>
                
                <div class="upload-area" id="upload-area">
                    <div style="margin-bottom: 15px;">📹 Drop your video file here</div>
                    <div style="font-size: 0.9rem; opacity: 0.8; margin-bottom: 15px;">
                        or click to browse your computer
                    </div>
                    <input type="file" id="video-input" class="file-input" accept="video/*">
                    <button class="upload-button" onclick="document.getElementById('video-input').click()">
                        Choose Video File
                    </button>
                </div>
                
                <div id="file-info" class="file-info" style="display: none;"></div>
                
                <button class="preload-button" id="process-upload" style="display: none;">
                    🚀 Start Analysis
                </button>
            </div>
            
            <!-- Preloaded Video Option -->
            <div class="option-card" id="preload-card">
                <div class="option-icon">🎬</div>
                <div class="option-title">Use Sample Video</div>
                <div class="option-description">
                    Try our preloaded sample video (vid_3.mov) to see the pipeline in action. 
                    Perfect for testing and demonstration.
                </div>
                
                <div style="margin: 20px 0; padding: 15px; background: rgba(76, 175, 80, 0.1); border-radius: 10px;">
                    <div style="font-weight: bold; margin-bottom: 10px;">📋 Sample Video: vid_3.mov</div>
                    <div style="font-size: 0.9rem; opacity: 0.9;">
                        • Short demonstration video<br>
                        • Contains bird movement<br>
                        • Optimized for testing
                    </div>
                </div>
                
                <button class="preload-button" id="process-preload">
                    🚀 Start Sample Analysis
                </button>
            </div>
            
            <!-- Test Data Option -->
            <div class="option-card" id="testdata-card">
                <div class="option-icon">⚡</div>
                <div class="option-title">Use Test Data</div>
                <div class="option-description">
                    Skip video processing and use existing test data to run the ML pipeline quickly. 
                    Perfect for rapid testing of clustering and fine-tuning.
                </div>
                
                <div style="margin: 20px 0; padding: 15px; background: rgba(255, 107, 53, 0.1); border-radius: 10px;">
                    <div style="font-weight: bold; margin-bottom: 10px;">⚡ Fast Mode: Existing Data</div>
                    <div style="font-size: 0.9rem; opacity: 0.9;">
                        • Uses pre-processed frames<br>
                        • Skips video analysis<br>
                        • ~2-3 minutes to complete
                    </div>
                </div>
                
                <button class="preload-button" id="process-testdata" style="background: linear-gradient(45deg, #FF6B35, #F7931E);">
                    ⚡ Start Fast Analysis
                </button>
            </div>
        </div>
        
        <!-- Status Container -->
        <div class="status-container" id="status-container">
            <div class="status-text" id="status-text">Initializing pipeline...</div>
            <div class="progress-bar">
                <div class="progress-fill" id="progress-fill"></div>
            </div>
            <div class="status-text" id="progress-text">0%</div>
        </div>
    </div>

    <script>
        let selectedFile = null;
        
        // File upload handling
        const videoInput = document.getElementById('video-input');
        const uploadArea = document.getElementById('upload-area');
        const fileInfo = document.getElementById('file-info');
        const processUploadBtn = document.getElementById('process-upload');
        const processPreloadBtn = document.getElementById('process-preload');
        const statusContainer = document.getElementById('status-container');
        const statusText = document.getElementById('status-text');
        const progressFill = document.getElementById('progress-fill');
        const progressText = document.getElementById('progress-text');
        
        // Drag and drop functionality
        uploadArea.addEventListener('dragover', (e) => {
            e.preventDefault();
            uploadArea.classList.add('dragover');
        });
        
        uploadArea.addEventListener('dragleave', () => {
            uploadArea.classList.remove('dragover');
        });
        
        uploadArea.addEventListener('drop', (e) => {
            e.preventDefault();
            uploadArea.classList.remove('dragover');
            const files = e.dataTransfer.files;
            if (files.length > 0) {
                handleFileSelect(files[0]);
            }
        });
        
        // File input change
        videoInput.addEventListener('change', (e) => {
            if (e.target.files.length > 0) {
                handleFileSelect(e.target.files[0]);
            }
        });
        
        function handleFileSelect(file) {
            selectedFile = file;
            
            // Validate file type
            const allowedTypes = ['video/mp4', 'video/avi', 'video/mov', 'video/mkv', 'video/wmv', 'video/flv', 'video/webm'];
            const fileExt = file.name.split('.').pop().toLowerCase();
            const allowedExts = ['mp4', 'avi', 'mov', 'mkv', 'wmv', 'flv', 'webm'];
            
            if (!allowedTypes.includes(file.type) && !allowedExts.includes(fileExt)) {
                alert('Please select a valid video file (MP4, AVI, MOV, MKV, WMV, FLV, WEBM)');
                return;
            }
            
            // Validate file size (500MB limit)
            if (file.size > 500 * 1024 * 1024) {
                alert('File size must be less than 500MB');
                return;
            }
            
            // Show file info
            fileInfo.innerHTML = \`
                <div><strong>📁 File:</strong> \${file.name}</div>
                <div><strong>📏 Size:</strong> \${(file.size / (1024 * 1024)).toFixed(2)} MB</div>
                <div><strong>🎬 Type:</strong> \${file.type || 'video/' + fileExt}</div>
            \`;
            fileInfo.style.display = 'block';
            processUploadBtn.style.display = 'inline-block';
        }
        
        // Process uploaded video
        processUploadBtn.addEventListener('click', () => {
            if (!selectedFile) return;
            
            const formData = new FormData();
            formData.append('video', selectedFile);
            
            startPipeline('upload', formData);
        });
        
        // Process preloaded video
        processPreloadBtn.addEventListener('click', () => {
            startPipeline('preload', null);
        });
        
        // Process test data
        const processTestdataBtn = document.getElementById('process-testdata');
        processTestdataBtn.addEventListener('click', () => {
            startPipeline('testdata', null);
        });
        
        function startPipeline(type, formData) {
            // Show status container
            statusContainer.classList.add('active');
            statusText.textContent = 'Starting Birds of Play pipeline...';
            progressFill.style.width = '5%';
            progressText.textContent = '5%';
            
            // Disable buttons
            processUploadBtn.disabled = true;
            processPreloadBtn.disabled = true;
            const processTestdataBtn = document.getElementById('process-testdata');
            processTestdataBtn.disabled = true;
            
            let endpoint;
            if (type === 'upload') {
                endpoint = '/api/process-upload';
            } else if (type === 'preload') {
                endpoint = '/api/process-preload';
            } else if (type === 'testdata') {
                endpoint = '/api/process-testdata';
            }
            
            fetch(endpoint, {
                method: 'POST',
                body: formData || null
            })
            .then(response => response.json())
            .then(data => {
                if (data.success) {
                    statusText.textContent = 'Pipeline started successfully!';
                    progressFill.style.width = '10%';
                    progressText.textContent = '10%';
                    
                    // Poll for status updates
                    pollPipelineStatus(data.pipelineId);
                } else {
                    throw new Error(data.error || 'Failed to start pipeline');
                }
            })
            .catch(error => {
                console.error('Error:', error);
                statusText.innerHTML = \`<span class="error-message">❌ Error: \${error.message}</span>\`;
                progressFill.style.width = '0%';
                progressText.textContent = '0%';
                
                // Re-enable buttons
                processUploadBtn.disabled = false;
                processPreloadBtn.disabled = false;
                const processTestdataBtn = document.getElementById('process-testdata');
                processTestdataBtn.disabled = false;
            });
        }
        
        function pollPipelineStatus(pipelineId) {
            const pollInterval = setInterval(() => {
                fetch(\`/api/pipeline-status/\${pipelineId}\`)
                .then(response => response.json())
                .then(data => {
                    if (data.status === 'completed') {
                        clearInterval(pollInterval);
                        statusText.innerHTML = \`<span class="success-message">✅ Pipeline completed successfully!</span>\`;
                        progressFill.style.width = '100%';
                        progressText.textContent = '100%';
                        
                        // Show navigation links
                        setTimeout(() => {
                            statusText.innerHTML += \`
                                <div style="margin-top: 20px;">
                                    <div style="margin: 10px 0; font-weight: bold;">🎉 Analysis Complete! View Results:</div>
                                    <div style="display: flex; gap: 10px; justify-content: center; flex-wrap: wrap;">
                                        <a href="http://localhost:3000" target="_blank" style="color: #FF6B35; text-decoration: none; font-weight: bold;">📹 Motion Detection</a>
                                        <a href="http://localhost:3001" target="_blank" style="color: #FF6B35; text-decoration: none; font-weight: bold;">🎯 Object Detection</a>
                                        <a href="http://localhost:3002" target="_blank" style="color: #FF6B35; text-decoration: none; font-weight: bold;">🔬 Bird Clustering</a>
                                        <a href="http://localhost:3003" target="_blank" style="color: #FF6B35; text-decoration: none; font-weight: bold;">🧠 Fine-Tuning</a>
                                    </div>
                                </div>
                            \`;
                        }, 2000);
                        
                    } else if (data.status === 'error') {
                        clearInterval(pollInterval);
                        statusText.innerHTML = \`<span class="error-message">❌ Pipeline failed: \${data.error}</span>\`;
                        progressFill.style.width = '0%';
                        progressText.textContent = '0%';
                        
                        // Re-enable buttons
                        processUploadBtn.disabled = false;
                        processPreloadBtn.disabled = false;
                        const processTestdataBtn = document.getElementById('process-testdata');
                        processTestdataBtn.disabled = false;
                        
                    } else if (data.status === 'running') {
                        const progress = Math.min(data.progress || 0, 95);
                        statusText.textContent = data.message || 'Processing...';
                        progressFill.style.width = progress + '%';
                        progressText.textContent = progress + '%';
                    }
                })
                .catch(error => {
                    console.error('Error polling status:', error);
                    clearInterval(pollInterval);
                    statusText.innerHTML = \`<span class="error-message">❌ Error checking status: \${error.message}</span>\`;
                    
                    // Re-enable buttons
                    processUploadBtn.disabled = false;
                    processPreloadBtn.disabled = false;
                    const processTestdataBtn = document.getElementById('process-testdata');
                    processTestdataBtn.disabled = false;
                });
            }, 2000);
        }
    </script>
</body>
</html>
    `);
});

// Process uploaded video
app.post('/api/process-upload', upload.single('video'), async (req, res) => {
    try {
        if (!req.file) {
            return res.status(400).json({ success: false, error: 'No video file uploaded' });
        }
        
        const pipelineId = `upload_${Date.now()}`;
        const videoPath = req.file.path;
        
        // Start pipeline in background using bash script to activate venv
        const pipeline = spawn('bash', ['-c', `source venv_ml/bin/activate && python test/full_pipeline_test.py --video "${videoPath}"`], {
            cwd: path.join(__dirname, '..'),
            detached: true,
            stdio: ['ignore', 'pipe', 'pipe']
        });
        
        // Handle spawn errors
        pipeline.on('error', (error) => {
            console.error(`Failed to start pipeline ${pipelineId}:`, error);
            updatePipelineStatus(pipelineId, 'error', 0, `Failed to start: ${error.message}`);
        });
        
        activePipelines.set(pipelineId, {
            process: pipeline,
            status: 'running',
            progress: 0,
            message: 'Starting motion detection...',
            videoPath: videoPath
        });
        
        // Monitor pipeline output
        monitorPipeline(pipelineId, pipeline);
        
        res.json({ success: true, pipelineId: pipelineId });
        
    } catch (error) {
        console.error('Error processing upload:', error);
        res.status(500).json({ success: false, error: error.message });
    }
});

// Process preloaded video
app.post('/api/process-preload', async (req, res) => {
    try {
        const pipelineId = `preload_${Date.now()}`;
        const videoPath = path.join(__dirname, '..', 'test', 'vid', 'vid_3.mov');
        
        // Check if preloaded video exists
        if (!fs.existsSync(videoPath)) {
            return res.status(400).json({ success: false, error: 'Preloaded video vid_3.mov not found' });
        }
        
        // Start pipeline in background using bash script to activate venv
        const pipeline = spawn('bash', ['-c', `source venv_ml/bin/activate && python test/full_pipeline_test.py --video "${videoPath}"`], {
            cwd: path.join(__dirname, '..'),
            detached: true,
            stdio: ['ignore', 'pipe', 'pipe']
        });
        
        // Handle spawn errors
        pipeline.on('error', (error) => {
            console.error(`Failed to start pipeline ${pipelineId}:`, error);
            updatePipelineStatus(pipelineId, 'error', 0, `Failed to start: ${error.message}`);
        });
        
        activePipelines.set(pipelineId, {
            process: pipeline,
            status: 'running',
            progress: 0,
            message: 'Starting motion detection...',
            videoPath: videoPath
        });
        
        // Monitor pipeline output
        monitorPipeline(pipelineId, pipeline);
        
        res.json({ success: true, pipelineId: pipelineId });
        
    } catch (error) {
        console.error('Error processing preload:', error);
        res.status(500).json({ success: false, error: error.message });
    }
});

// Process test data (skip video processing)
app.post('/api/process-testdata', async (req, res) => {
    try {
        const pipelineId = `testdata_${Date.now()}`;
        
        // Check if test data exists
        const testDataPath = path.join(__dirname, '..', 'data');
        if (!fs.existsSync(testDataPath)) {
            return res.status(400).json({ success: false, error: 'Test data directory not found' });
        }
        
        // Start pipeline in background using bash script to activate venv, skipping video steps
        const pipeline = spawn('bash', ['-c', `source venv_ml/bin/activate && python test/full_pipeline_test.py --skip-video`], {
            cwd: path.join(__dirname, '..'),
            detached: true,
            stdio: ['ignore', 'pipe', 'pipe']
        });
        
        // Handle spawn errors
        pipeline.on('error', (error) => {
            console.error(`Failed to start pipeline ${pipelineId}:`, error);
            updatePipelineStatus(pipelineId, 'error', 0, `Failed to start: ${error.message}`);
        });
        
        activePipelines.set(pipelineId, {
            process: pipeline,
            status: 'running',
            progress: 0,
            message: 'Starting with existing test data...',
            videoPath: 'test data'
        });
        
        // Monitor pipeline output
        monitorPipeline(pipelineId, pipeline);
        
        res.json({ success: true, pipelineId: pipelineId });
        
    } catch (error) {
        console.error('Error processing test data:', error);
        res.status(500).json({ success: false, error: error.message });
    }
});

// Get pipeline status
app.get('/api/pipeline-status/:pipelineId', (req, res) => {
    const pipelineId = req.params.pipelineId;
    const pipeline = activePipelines.get(pipelineId);
    
    if (!pipeline) {
        return res.status(404).json({ success: false, error: 'Pipeline not found' });
    }
    
    res.json({
        success: true,
        status: pipeline.status,
        progress: pipeline.progress,
        message: pipeline.message,
        error: pipeline.error
    });
});

function monitorPipeline(pipelineId, process) {
    let output = '';
    
    process.stdout.on('data', (data) => {
        output += data.toString();
        
        // Parse progress from output
        const lines = output.split('\\n');
        const lastLine = lines[lines.length - 2] || '';
        
        // Update progress based on pipeline steps
        if (lastLine.includes('Step 1:')) {
            updatePipelineStatus(pipelineId, 'running', 10, 'Clearing data...');
        } else if (lastLine.includes('Step 2:')) {
            updatePipelineStatus(pipelineId, 'running', 20, 'Clearing MongoDB...');
        } else if (lastLine.includes('Step 3:')) {
            updatePipelineStatus(pipelineId, 'running', 30, 'Running motion detection...');
        } else if (lastLine.includes('Step 4:')) {
            updatePipelineStatus(pipelineId, 'running', 40, 'Verifying frames...');
        } else if (lastLine.includes('Step 5:')) {
            updatePipelineStatus(pipelineId, 'running', 50, 'Extracting regions...');
        } else if (lastLine.includes('Step 6:')) {
            updatePipelineStatus(pipelineId, 'running', 60, 'Running object detection...');
        } else if (lastLine.includes('Step 7:')) {
            updatePipelineStatus(pipelineId, 'running', 70, 'Starting web servers...');
        } else if (lastLine.includes('Step 8:')) {
            updatePipelineStatus(pipelineId, 'running', 80, 'Extracting objects...');
        } else if (lastLine.includes('Step 9:')) {
            updatePipelineStatus(pipelineId, 'running', 85, 'Initializing clustering...');
        } else if (lastLine.includes('Step 10:')) {
            updatePipelineStatus(pipelineId, 'running', 90, 'Training supervised model...');
        } else if (lastLine.includes('ALL TESTS PASSED')) {
            updatePipelineStatus(pipelineId, 'completed', 100, 'Pipeline completed successfully!');
        }
    });
    
    process.stderr.on('data', (data) => {
        const errorMsg = data.toString();
        console.error(`Pipeline ${pipelineId} error:`, errorMsg);
        updatePipelineStatus(pipelineId, 'error', 0, `Pipeline error: ${errorMsg}`);
    });
    
    process.on('close', (code) => {
        const pipeline = activePipelines.get(pipelineId);
        if (!pipeline) return;
        
        if (code === 0) {
            updatePipelineStatus(pipelineId, 'completed', 100, 'Pipeline completed successfully!');
        } else {
            updatePipelineStatus(pipelineId, 'error', 0, `Pipeline exited with code ${code || 'unknown'}`);
        }
    });
}

function updatePipelineStatus(pipelineId, status, progress, message) {
    const pipeline = activePipelines.get(pipelineId);
    if (pipeline) {
        pipeline.status = status;
        pipeline.progress = progress;
        pipeline.message = message;
        activePipelines.set(pipelineId, pipeline);
    }
}

// Clean up completed pipelines
setInterval(() => {
    for (const [pipelineId, pipeline] of activePipelines.entries()) {
        if (pipeline.status === 'completed' || pipeline.status === 'error') {
            activePipelines.delete(pipelineId);
        }
    }
}, 30000); // Clean up every 30 seconds

app.listen(PORT, () => {
    console.log(`🌐 Birds of Play Landing Page running on http://localhost:${PORT}`);
    console.log(`📋 Features:`);
    console.log(`   • Video upload with drag & drop`);
    console.log(`   • Preloaded sample video (vid_3.mov)`);
    console.log(`   • Real-time pipeline progress tracking`);
    console.log(`   • Direct links to all analysis browsers`);
    console.log(`   • 500MB upload limit for user videos`);
});
