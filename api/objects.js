// Object Detection/Consolidated Regions Viewer - Vercel Serverless Function
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
            region_id: "region_001",
            detection_id: "frame_26_region_0_det_0",
            confidence: 0.85,
            class_name: "bird",
            bbox: { x: 349, y: 796, width: 244, height: 259 },
            timestamp: "2025-01-25T03:29:37.000Z"
        },
        {
            _id: "det_002", 
            region_id: "region_002",
            detection_id: "frame_27_region_0_det_0",
            confidence: 0.92,
            class_name: "bird",
            bbox: { x: 400, y: 600, width: 180, height: 200 },
            timestamp: "2025-01-25T03:29:38.000Z"
        },
        {
            _id: "det_003",
            region_id: "region_003", 
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
            detections: mockDetectionData,
            totalDetections: 25,
            regionsAnalyzed: 74,
            averageConfidence: 0.84,
            speciesClusters: 4
        });
        return;
    }

    // Main object detection viewer HTML
    res.status(200).send(\`
<!DOCTYPE html>
<html>
<head>
    <title>🦅 Birds of Play - Object Detection Viewer</title>
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <style>
        body { 
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            margin: 0;
            padding: 20px; 
            background: linear-gradient(135deg, #1a1a1a 0%, #2d2d2d 100%);
            color: white; 
            min-height: 100vh;
        }
        .container { max-width: 1200px; margin: 0 auto; }
        .header {
            text-align: center;
            margin-bottom: 40px;
            padding: 30px;
            background: rgba(255, 255, 255, 0.1);
            border-radius: 15px;
            backdrop-filter: blur(10px);
        }
        h1 { 
            color: #FF6B35; 
            font-size: 2.5rem;
            margin: 0 0 10px 0;
            text-shadow: 2px 2px 4px rgba(0,0,0,0.3);
        }
        .subtitle {
            font-size: 1.2rem;
            opacity: 0.9;
            margin: 0;
        }
        .nav-links {
            display: flex;
            justify-content: center;
            gap: 15px;
            margin: 20px 0;
            flex-wrap: wrap;
        }
        .nav-link {
            background: rgba(255, 107, 53, 0.2);
            color: #FF6B35;
            padding: 10px 20px;
            border-radius: 25px;
            text-decoration: none;
            font-weight: bold;
            transition: all 0.3s ease;
            border: 2px solid rgba(255, 107, 53, 0.3);
        }
        .nav-link:hover, .nav-link.active {
            background: #FF6B35;
            color: white;
            transform: translateY(-2px);
            box-shadow: 0 5px 15px rgba(255, 107, 53, 0.4);
        }
        .stats-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 20px;
            margin: 30px 0;
        }
        .stat-card {
            background: rgba(255, 255, 255, 0.1);
            border-radius: 15px;
            padding: 25px;
            text-align: center;
            border: 1px solid rgba(255, 255, 255, 0.2);
            transition: transform 0.3s ease;
        }
        .stat-card:hover {
            transform: translateY(-5px);
        }
        .stat-number {
            font-size: 2.5rem;
            font-weight: bold;
            color: #4CAF50;
            margin-bottom: 10px;
        }
        .stat-label {
            font-size: 1.1rem;
            opacity: 0.9;
        }
        .detection-grid { 
            display: grid; 
            grid-template-columns: repeat(auto-fill, minmax(350px, 1fr)); 
            gap: 25px; 
            margin: 30px 0;
        }
        .detection-card {
            background: rgba(255, 255, 255, 0.1);
            border-radius: 15px;
            padding: 25px;
            border: 1px solid rgba(255, 255, 255, 0.2);
            transition: all 0.3s ease;
        }
        .detection-card:hover {
            transform: translateY(-5px);
            border-color: #4CAF50;
        }
        .detection-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 20px;
            padding-bottom: 15px;
            border-bottom: 1px solid rgba(255, 255, 255, 0.2);
        }
        .detection-id {
            font-weight: bold;
            color: #4CAF50;
            font-size: 1.1rem;
        }
        .confidence-badge {
            background: linear-gradient(45deg, #4CAF50, #45a049);
            color: white;
            padding: 5px 12px;
            border-radius: 15px;
            font-size: 0.9rem;
            font-weight: bold;
        }
        .demo-notice {
            background: rgba(33, 150, 243, 0.1);
            border: 1px solid rgba(33, 150, 243, 0.3);
            border-radius: 10px;
            padding: 20px;
            margin: 20px 0;
            text-align: center;
        }
        .demo-notice h3 {
            color: #2196F3;
            margin: 0 0 10px 0;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🎯 Object Detection Viewer</h1>
            <p class="subtitle">YOLO11 Detection Results from DBSCAN Consolidated Regions</p>
            
            <div class="nav-links">
                <a href="/api/motion" class="nav-link">📹 Motion Detection</a>
                <a href="/api/objects" class="nav-link active">🎯 Object Detection</a>
                <a href="/api/clustering" class="nav-link">🔬 Bird Clustering</a>
                <a href="/api/finetuning" class="nav-link">🧠 Fine-Tuning</a>
                <a href="/" class="nav-link">🏠 Home</a>
            </div>
        </div>

        <div class="demo-notice">
            <h3>🎯 YOLO11 + DBSCAN Integration</h3>
            <p>These are high-confidence bird detections (>50%) found within DBSCAN consolidated motion regions. Our pipeline extracted 74 regions and found 25 bird detections.</p>
        </div>

        <div class="stats-grid">
            <div class="stat-card">
                <div class="stat-number">74</div>
                <div class="stat-label">Regions Analyzed</div>
            </div>
            <div class="stat-card">
                <div class="stat-number">25</div>
                <div class="stat-label">Bird Detections</div>
            </div>
            <div class="stat-card">
                <div class="stat-number">84%</div>
                <div class="stat-label">Avg Confidence</div>
            </div>
            <div class="stat-card">
                <div class="stat-number">4</div>
                <div class="stat-label">Species Clusters</div>
            </div>
        </div>

        <h2 style="color: #4CAF50; text-align: center; margin: 40px 0 30px 0;">🔍 High-Confidence Bird Detections</h2>
        
        <div class="detection-grid">
            \${mockDetectionData.map(detection => \`
                <div class="detection-card">
                    <div class="detection-header">
                        <div class="detection-id">\${detection.detection_id}</div>
                        <div class="confidence-badge">\${Math.round(detection.confidence * 100)}%</div>
                    </div>
                    
                    <div style="background: rgba(33, 150, 243, 0.1); padding: 15px; border-radius: 8px; margin: 15px 0;">
                        <div style="font-family: 'Courier New', monospace; font-size: 0.95rem; color: #4CAF50; margin-bottom: 8px;">
                            Bounding Box: (\${detection.bbox.x}, \${detection.bbox.y})
                        </div>
                        <div style="font-size: 0.9rem; opacity: 0.9;">
                            Size: \${detection.bbox.width} × \${detection.bbox.height} pixels
                        </div>
                        <div style="margin-top: 8px; font-size: 0.85rem; opacity: 0.8;">
                            Area: \${detection.bbox.width * detection.bbox.height} px²
                        </div>
                    </div>
                    
                    <div style="text-align: center; margin-top: 15px; font-size: 0.9rem; opacity: 0.8;">
                        Detected: \${new Date(detection.timestamp).toLocaleString()}
                    </div>
                </div>
            \`).join('')}
        </div>
    </div>
</body>
</html>
    \`);
}
