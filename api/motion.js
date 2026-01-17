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

    // Real motion detection frames from pipeline test
    const frameImages = [
        '02e918ac-bc6b-4ea2-a7e3-f71f98e403d6.jpg',
        '038bb632-9573-4a52-883f-e3c45399a83c.jpg',
        '04fb3df4-436b-477c-8b79-e20135d03465.jpg',
        '06fb2643-949d-452a-9b12-c864bf832226.jpg',
        '09e02fc9-68ed-4e29-bd75-2264606f9d24.jpg',
        '0a8f77ed-2fa4-4f80-bce3-9c0143b198ac.jpg',
        '0abde64b-274b-4474-91ca-d6af636db7af.jpg',
        '0b6383c9-b22d-4fcb-bf8f-cd216c788765.jpg',
        '0b85cc88-45ed-48a3-a72a-4499b2b362c8.jpg',
        '0bd4015e-16aa-4cfe-83a4-95a8a3555345.jpg',
        '0c2ad07f-41d6-4979-a620-b7824b6f6ecb.jpg',
        '0c72397b-ac0e-41c5-ae89-94fba724e06d.jpg',
        '0cb06095-c23d-48c5-8c6c-c97819cfe17f.jpg',
        '0dd66803-eef9-485c-8ba4-a105e29d58f7.jpg',
        '0e18b230-d77e-4ddf-b1de-a4458e4739c1.jpg',
        '0f55a06f-2dea-43ab-a881-a56fa3501618.jpg',
        '0f79f343-5e56-4546-9a9d-556fe6c5d2c2.jpg',
        '0fbd4af0-1dd9-470a-ad62-600271b55173.jpg',
        '118cf4a7-6aa3-4e43-a093-322f2e873c9a.jpg',
        '11a6a99e-177f-4f27-811f-82f28b0d9492.jpg'
    ];

    // Generate frame gallery HTML (matching localhost grid style)
    const imageGallery = frameImages.map((image, index) => `
        <div class="frame-card">
            <img src="/images/frames/${image}" alt="Motion detection frame ${index + 1}" class="frame-image" />
            <div class="frame-number">Frame ${index + 1}</div>
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
            display: flex;
            flex-wrap: wrap;
            gap: 10px;
            justify-content: center;
            margin: 20px 0;
        }
        .nav-link {
            background: linear-gradient(45deg, #FF6B35, #F7931E);
            color: white;
            padding: 10px 16px;
            border-radius: 999px;
            text-decoration: none;
            font-size: 0.9rem;
            font-weight: bold;
            transition: transform 0.2s ease, box-shadow 0.2s ease, opacity 0.2s ease;
            box-shadow: 0 6px 14px rgba(255, 107, 53, 0.25);
        }
        .nav-link:hover {
            transform: translateY(-2px);
            box-shadow: 0 8px 16px rgba(255, 107, 53, 0.4);
        }
        .nav-link-active {
            box-shadow: 0 0 0 2px rgba(255, 255, 255, 0.8), 0 8px 18px rgba(255, 107, 53, 0.55);
            cursor: default;
            pointer-events: none;
        }
        .image-grid { 
            display: grid; 
            grid-template-columns: repeat(auto-fill, minmax(300px, 1fr)); 
            gap: 15px; 
            margin: 20px 0;
        }
        .frame-card {
            background: rgba(255, 255, 255, 0.05);
            border-radius: 8px;
            overflow: hidden;
            border: 1px solid rgba(255, 255, 255, 0.1);
            transition: transform 0.2s ease, border-color 0.2s ease;
        }
        .frame-card:hover {
            transform: translateY(-5px);
            border-color: #FF6B35;
        }
        .frame-image {
            width: 100%;
            height: auto;
            display: block;
        }
        .frame-number {
            padding: 10px;
            text-align: center;
            font-size: 0.9rem;
            color: #FFD700;
            background: rgba(0, 0, 0, 0.3);
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
            <a href="/" class="nav-link">🏠 Home</a>
            <a href="/api/motion" class="nav-link nav-link-active" aria-current="page">📹 Motion Detection</a>
            <a href="/api/objects" class="nav-link">🎯 Object Detection</a>
            <a href="/api/clustering" class="nav-link">🔬 Bird Clustering</a>
            <a href="/api/finetuning" class="nav-link">🧠 Fine-Tuning</a>
        </div>

        <h2>Motion Detection Results</h2>
        <p>Processed frames from DBSCAN motion detection pipeline showing gray boxes (individual motion) and red boxes (consolidated regions):</p>
        
        <div class="image-grid">
            ${imageGallery}
        </div>
        
        <div style="margin-top: 30px; padding: 20px; background: rgba(255, 107, 53, 0.1); border-radius: 8px; border: 1px solid rgba(255, 107, 53, 0.3);">
            <h3 style="color: #FFD700; margin-top: 0;">📊 Pipeline Statistics</h3>
            <p><strong>Total Frames Processed:</strong> 125</p>
            <p><strong>Frames with Motion:</strong> 125 (100%)</p>
            <p><strong>DBSCAN Consolidated Regions:</strong> 125</p>
            <p><strong>Average Objects per Region:</strong> ~15</p>
            <p style="margin-top: 15px; opacity: 0.9;">
                Each frame shows the result of DBSCAN clustering with overlap-aware distance metrics,
                consolidating nearby motion objects into coherent regions for bird detection.
            </p>
        </div>
    </div>
</body>
</html>
    `);
}