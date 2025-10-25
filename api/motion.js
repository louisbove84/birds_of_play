// Motion Detection Viewer - Vercel Serverless Function
export default function handler(req, res) {
    // Set CORS headers
    res.setHeader('Access-Control-Allow-Origin', '*');
    res.setHeader('Access-Control-Allow-Methods', 'GET, POST, OPTIONS');
    res.setHeader('Access-Control-Allow-Headers', 'Content-Type');

    if (req.method === 'OPTIONS') {
        res.status(200).end();
        return;
    }

    // Mock frame data from our pipeline test
    const mockFrameData = [
        {
            _id: "frame_001",
            timestamp: "2025-01-25T03:29:37.000Z",
            metadata: {
                frame_count: 26,
                motion_detected: true,
                motion_regions: 3,
                consolidated_regions_count: 1,
                consolidated_regions: [{
                    x: 349, y: 796, width: 244, height: 259, object_count: 17
                }]
            }
        },
        {
            _id: "frame_002",
            timestamp: "2025-01-25T03:29:38.000Z",
            metadata: {
                frame_count: 27,
                motion_detected: true,
                motion_regions: 2,
                consolidated_regions_count: 1,
                consolidated_regions: [{
                    x: 400, y: 600, width: 180, height: 200, object_count: 12
                }]
            }
        }
    ];

    if (req.url === '/api/frames') {
        res.status(200).json({
            status: 'success',
            frames: mockFrameData
        });
        return;
    }

    // Real motion processing images from test results
    const motionImages = [
        '1_pair_0_1_motion_processing.jpg',
        '2_pair_0_1_motion_processing.jpg', 
        '3_pair_0_1_motion_processing.jpg',
        '2_pair_0_1_consolidation.jpg',
        '3_pair_0_1_consolidation.jpg'
    ];

    // Generate image gallery HTML
    const imageGallery = motionImages.map((image, index) => `
        <div class="image-card">
            <div class="image-header">
                <div class="image-title">${image.includes('consolidation') ? 'DBSCAN Consolidation' : 'Motion Processing'}</div>
                <div class="image-number">Result ${index + 1}</div>
            </div>
            <img src="/images/motion/${image}" alt="Motion detection result" class="result-image" />
            <div class="image-description">
                ${image.includes('consolidation') ? 
                    'Shows consolidated motion regions using DBSCAN clustering with overlap-aware distance metrics.' :
                    'Shows individual motion detection with bounding boxes around detected movement.'}
            </div>
        </div>
    `).join('');

    // Main motion detection viewer HTML
    res.status(200).send(`
<!DOCTYPE html>
<html>
<head>
    <title>🦅 Birds of Play - Motion Detection Viewer</title>
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <style>
        body { 
            font-family: Arial, sans-serif;
            margin: 20px;
            background: #1a1a1a;
            color: white;
        }
        .container { max-width: 1200px; margin: 0 auto; }
        h1 { 
            color: #FF6B35; 
            text-align: center;
        }
        .nav-links {
            margin-top: 1rem;
            font-size: 0.9em;
            background: rgba(255, 255, 255, 0.1);
            padding: 0.5rem 1rem;
            border-radius: 8px;
            display: inline-block;
            text-align: center;
        }
        .nav-links a {
            color: #FF6B35;
            text-decoration: none;
            margin: 0 10px;
        }
        .nav-links a:hover {
            text-decoration: underline;
        }
        .image-grid { 
            display: grid; 
            grid-template-columns: repeat(auto-fill, minmax(400px, 1fr)); 
            gap: 20px; 
            margin: 20px 0;
        }
        .image-card {
            background: rgba(255, 255, 255, 0.1);
            border-radius: 8px;
            padding: 20px;
            border: 1px solid rgba(255, 255, 255, 0.2);
        }
        .result-image {
            width: 100%;
            height: auto;
            border-radius: 8px;
            margin: 10px 0;
        }
        .image-description {
            font-size: 0.9rem;
            opacity: 0.9;
            margin-top: 10px;
            line-height: 1.4;
        }
        .image-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 15px;
            padding-bottom: 10px;
            border-bottom: 1px solid rgba(255, 255, 255, 0.2);
        }
        .image-title {
            font-weight: bold;
            color: #FF6B35;
        }
        .image-number {
            font-size: 0.9rem;
            opacity: 0.8;
        }
        .motion-info {
            display: grid;
            grid-template-columns: repeat(2, 1fr);
            gap: 10px;
            margin: 15px 0;
        }
        .info-item {
            background: rgba(0, 0, 0, 0.3);
            padding: 10px;
            border-radius: 4px;
            text-align: center;
        }
        .info-label {
            font-size: 0.9rem;
            opacity: 0.8;
            margin-bottom: 5px;
        }
        .info-value {
            font-size: 1.2rem;
            font-weight: bold;
            color: #4CAF50;
        }
        .regions-list {
            margin-top: 15px;
        }
        .regions-list h4 {
            color: #FFD700;
            margin: 10px 0;
        }
        .region-item {
            background: rgba(255, 107, 53, 0.1);
            border: 1px solid rgba(255, 107, 53, 0.3);
            border-radius: 4px;
            padding: 10px;
            margin: 8px 0;
        }
        .region-coords {
            font-family: monospace;
            color: #FFD700;
            margin-bottom: 5px;
        }
        .region-objects {
            font-size: 0.9rem;
            opacity: 0.9;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>📹 Motion Detection Viewer</h1>
        
        <div class="nav-links">
            <a href="/">🏠 Home</a>
            <a href="/api/motion">📹 Motion Detection</a>
            <a href="/api/objects">🎯 Object Detection</a>
            <a href="/api/clustering">🔬 Bird Clustering</a>
            <a href="/api/finetuning">🧠 Fine-Tuning</a>
        </div>

        <h2>Motion Detection Results</h2>
        <p>Real images from DBSCAN motion detection pipeline test:</p>
        
        <div class="image-grid">
            ${imageGallery}
        </div>
    </div>
</body>
</html>
    `);
}