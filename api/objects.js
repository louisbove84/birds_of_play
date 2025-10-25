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

    // Real bird detection images from test results
    const birdImages = [
        '1306aa44-2d7a-4d8d-aa1c-e2c511c5a3c6_0_obj_0.jpg',
        '1b81c650-c077-4af0-bc99-fb8848069b26_0_obj_0.jpg',
        '31556a49-c14c-46d7-89be-7a9f7633b797_0_obj_0.jpg',
        '324d2e91-c8fd-4d00-993e-663bd9c2434a_0_obj_0.jpg',
        '32e8027f-c327-464e-b936-13e5673bee94_0_obj_0.jpg',
        '4986359b-6a9c-4bff-8a80-cbe4115f96d8_0_obj_0.jpg',
        '63f05dde-3085-4f21-84c4-3b94303997db_0_obj_0.jpg',
        '709a8042-d5ae-4908-956e-38d039bdb6bd_0_obj_0.jpg',
        '75a2e6e0-941a-4cfe-adc0-20a6d3cc82d8_0_obj_0.jpg',
        '7799a21a-b4c1-41de-b9ff-0675285955ae_0_obj_0.jpg',
        '818458ae-43d2-40f8-9ef1-bce9b3351508_0_obj_0.jpg',
        '88d644e2-a27f-4d6e-b112-6755f76d5ed5_0_obj_0.jpg'
    ];

    // Generate bird detection gallery HTML
    const birdGallery = birdImages.map((image, index) => `
        <div class="detection-card">
            <div class="detection-header">
                <div class="detection-id">Bird Detection ${index + 1}</div>
                <div class="confidence-badge">High Confidence</div>
            </div>
            <img src="/images/objects/${image}" alt="Bird detection" class="bird-image" />
            <div class="detection-info">
                Extracted bird object from YOLO11 analysis of DBSCAN consolidated regions.
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
        .bird-image {
            width: 100%;
            height: 200px;
            object-fit: cover;
            border-radius: 8px;
            margin: 10px 0;
        }
        .detection-info {
            font-size: 0.9rem;
            opacity: 0.9;
            margin-top: 10px;
            line-height: 1.4;
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
        <p>Real bird objects extracted from YOLO11 analysis:</p>
        
        <div class="detection-grid">
            ${birdGallery}
        </div>
    </div>
</body>
</html>
    `);
}