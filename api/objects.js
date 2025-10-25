// Object Detection Viewer - Vercel Serverless Function
export default function handler(req, res) {
    // Set CORS headers
    res.setHeader('Access-Control-Allow-Origin', '*');
    res.setHeader('Access-Control-Allow-Methods', 'GET, POST, OPTIONS');
    res.setHeader('Access-Control-Allow-Headers', 'Content-Type');
    
    if (req.method === 'OPTIONS') {
        res.status(200).end();
        return;
    }

    // Mock detection data from our pipeline test
    const mockDetectionData = [
        {
            _id: "det_001",
            detection_id: "frame_26_region_0_det_0",
            confidence: 0.85,
            class_name: "bird",
            bbox: { x: 349, y: 796, width: 244, height: 259 },
            timestamp: "2025-01-25T03:29:37.000Z"
        },
        {
            _id: "det_002", 
            detection_id: "frame_27_region_0_det_0",
            confidence: 0.92,
            class_name: "bird",
            bbox: { x: 400, y: 600, width: 180, height: 200 },
            timestamp: "2025-01-25T03:29:38.000Z"
        },
        {
            _id: "det_003",
            detection_id: "frame_28_region_0_det_0",
            confidence: 0.78,
            class_name: "bird",
            bbox: { x: 320, y: 720, width: 160, height: 180 },
            timestamp: "2025-01-25T03:29:39.000Z"
        }
    ];

    if (req.url === '/api/detections') {
        res.status(200).json({
            status: 'success',
            detections: mockDetectionData
        });
        return;
    }

    // Generate detection cards HTML
    const detectionCards = mockDetectionData.map(detection => `
        <div class="detection-card">
            <div class="detection-header">
                <div class="detection-id">${detection.detection_id}</div>
                <div class="confidence-badge">${Math.round(detection.confidence * 100)}%</div>
            </div>
            
            <div class="bbox-info">
                <div class="bbox-coords">
                    Bounding Box: (${detection.bbox.x}, ${detection.bbox.y})
                </div>
                <div class="bbox-size">
                    Size: ${detection.bbox.width} × ${detection.bbox.height} pixels
                </div>
            </div>
            
            <div class="timestamp">
                Detected: ${new Date(detection.timestamp).toLocaleString()}
            </div>
        </div>
    `).join('');

    // Main object detection viewer HTML
    res.status(200).send(`
<!DOCTYPE html>
<html>
<head>
    <title>🦅 Birds of Play - Object Detection Viewer</title>
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
        .detection-grid { 
            display: grid; 
            grid-template-columns: repeat(auto-fill, minmax(300px, 1fr)); 
            gap: 20px; 
            margin: 20px 0;
        }
        .detection-card {
            background: rgba(255, 255, 255, 0.1);
            border-radius: 8px;
            padding: 20px;
            border: 1px solid rgba(255, 255, 255, 0.2);
        }
        .detection-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 15px;
            padding-bottom: 10px;
            border-bottom: 1px solid rgba(255, 255, 255, 0.2);
        }
        .detection-id {
            font-weight: bold;
            color: #4CAF50;
        }
        .confidence-badge {
            background: #4CAF50;
            color: white;
            padding: 4px 8px;
            border-radius: 4px;
            font-size: 0.9rem;
            font-weight: bold;
        }
        .bbox-info {
            background: rgba(76, 175, 80, 0.1);
            border: 1px solid rgba(76, 175, 80, 0.3);
            border-radius: 4px;
            padding: 10px;
            margin: 10px 0;
        }
        .bbox-coords {
            font-family: monospace;
            color: #4CAF50;
            margin-bottom: 5px;
        }
        .bbox-size {
            font-size: 0.9rem;
            opacity: 0.9;
        }
        .timestamp {
            text-align: center;
            margin-top: 10px;
            font-size: 0.9rem;
            opacity: 0.8;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🎯 Object Detection Viewer</h1>
        
        <div class="nav-links">
            <a href="/">🏠 Home</a>
            <a href="/api/motion">📹 Motion Detection</a>
            <a href="/api/objects">🎯 Object Detection</a>
            <a href="/api/clustering">🔬 Bird Clustering</a>
            <a href="/api/finetuning">🧠 Fine-Tuning</a>
        </div>

        <h2>High-Confidence Bird Detections</h2>
        
        <div class="detection-grid">
            ${detectionCards}
        </div>
    </div>
</body>
</html>
    `);
}