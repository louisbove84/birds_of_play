// Main landing page for Birds of Play - Vercel Serverless Function
export default function handler(req, res) {
    // Set CORS headers
    res.setHeader('Access-Control-Allow-Origin', '*');
    res.setHeader('Access-Control-Allow-Methods', 'GET, POST, OPTIONS');
    res.setHeader('Access-Control-Allow-Headers', 'Content-Type');

    if (req.method === 'OPTIONS') {
        res.status(200).end();
        return;
    }

    if (req.url === '/api/health') {
        res.status(200).json({
            status: 'healthy',
            timestamp: new Date().toISOString(),
            version: '1.0.1'
        });
        return;
    }

    if (req.url === '/api/pipeline-status') {
        res.status(200).json({
            status: 'success',
            lastRun: '2025-01-25T03:29:37.000Z',
            results: {
                framesProcessed: 65,
                motionRegionsDetected: 65,
                consolidatedRegions: 65,
                yoloDetections: 25,
                birdSpeciesClusters: 4,
                modelAccuracy: 0.80,
                executionTime: '337.8 seconds'
            }
        });
        return;
    }

    // Main landing page HTML
    res.status(200).send(`
    <!DOCTYPE html>
    <html lang="en">
    <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>🦅 Birds of Play - DBSCAN Motion Detection</title>
        <style>
            body {
                font-family: Arial, sans-serif;
                margin: 20px;
                background: #1a1a1a;
                color: white;
            }
            .container {
                max-width: 1200px;
                margin: 0 auto;
                text-align: center;
            }
            h1 {
                color: #FF6B35;
                font-size: 3rem;
                margin-bottom: 20px;
            }
            .subtitle {
                font-size: 1.2rem;
                margin-bottom: 40px;
                opacity: 0.9;
            }
            .nav-links {
                margin: 30px 0;
                display: flex;
                justify-content: center;
                gap: 20px;
                flex-wrap: wrap;
            }
            .nav-link {
                background: #FF6B35;
                color: white;
                padding: 12px 24px;
                border-radius: 8px;
                text-decoration: none;
                font-weight: bold;
                transition: background 0.3s ease;
            }
            .nav-link:hover {
                background: #e55a2b;
            }
            .features {
                display: grid;
                grid-template-columns: repeat(auto-fit, minmax(300px, 1fr));
                gap: 30px;
                margin: 40px 0;
                text-align: left;
            }
            .feature-card {
                background: rgba(255,255,255,0.1);
                border-radius: 8px;
                padding: 30px;
                border: 1px solid rgba(255,255,255,0.2);
            }
            .feature-card h3 {
                color: #FFD700;
                margin-bottom: 15px;
            }
            .feature-card p {
                line-height: 1.6;
                opacity: 0.9;
            }
            .demo-section {
                background: rgba(255,255,255,0.1);
                border-radius: 8px;
                padding: 40px;
                margin: 40px 0;
            }
            .demo-section h2 {
                color: #FFD700;
                margin-bottom: 20px;
            }
            .demo-results {
                display: grid;
                grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
                gap: 20px;
                margin: 20px 0;
            }
            .result-card {
                background: rgba(0,0,0,0.2);
                border-radius: 8px;
                padding: 20px;
                border: 1px solid rgba(255,255,255,0.1);
            }
            .result-card h4 {
                color: #4CAF50;
                margin-bottom: 10px;
            }
            .github-link {
                display: inline-block;
                background: #333;
                color: white;
                padding: 12px 24px;
                border-radius: 8px;
                text-decoration: none;
                margin: 20px 10px;
                transition: background 0.3s ease;
            }
            .github-link:hover {
                background: #555;
            }
        </style>
    </head>
    <body>
        <div class="container">
            <h1>🦅 Birds of Play</h1>
            <p class="subtitle">Advanced Motion Detection with DBSCAN Clustering</p>

            <div class="nav-links">
                <a href="/api/motion" class="nav-link">📹 Motion Detection</a>
                <a href="/api/objects" class="nav-link">🎯 Object Detection</a>
                <a href="/api/clustering" class="nav-link">🔬 Bird Clustering</a>
                <a href="/api/finetuning" class="nav-link">🧠 Fine-Tuning</a>
            </div>

            <div class="demo-section">
                <h2>Latest Pipeline Test Results</h2>
                <p>Full pipeline test completed successfully with DBSCAN implementation!</p>
                
                <div class="demo-results">
                    <div class="result-card">
                        <h4>📹 Motion Detection</h4>
                        <p><strong>65 frames</strong> processed from test video</p>
                        <p>All frames had motion regions detected</p>
                    </div>
                    <div class="result-card">
                        <h4>🔗 DBSCAN Clustering</h4>
                        <p><strong>3 motion objects</strong> → <strong>1 consolidated region</strong></p>
                        <p>244×259 pixels containing 17 objects</p>
                    </div>
                    <div class="result-card">
                        <h4>🎯 YOLO11 Detection</h4>
                        <p><strong>74 regions</strong> extracted and analyzed</p>
                        <p><strong>25 high-confidence</strong> bird detections</p>
                    </div>
                    <div class="result-card">
                        <h4>🧠 ML Pipeline</h4>
                        <p><strong>4 bird species</strong> clusters identified</p>
                        <p><strong>80% test accuracy</strong> on trained model</p>
                    </div>
                </div>
            </div>

            <div class="features">
                <div class="feature-card">
                    <h3>🔍 DBSCAN Clustering</h3>
                    <p>Advanced clustering algorithm that groups motion objects based on overlap-aware distance metrics. Perfect for handling overlapping bounding boxes like small birds inside larger motion regions.</p>
                </div>
                <div class="feature-card">
                    <h3>📊 No Size Constraints</h3>
                    <p>Regions are created purely based on spatial clustering without artificial size limitations. This allows for more natural grouping of motion objects.</p>
                </div>
                <div class="feature-card">
                    <h3>🎯 Smart Distance Calculation</h3>
                    <p>Combines bounding box overlap ratio with edge-to-edge distance for intelligent motion object grouping. Weighted combination ensures optimal clustering results.</p>
                </div>
                <div class="feature-card">
                    <h3>🔄 Full Pipeline Integration</h3>
                    <p>Seamlessly integrated with MongoDB storage, YOLO11 detection, machine learning clustering, and web visualization interfaces.</p>
                </div>
            </div>

            <div style="margin-top: 40px;">
                <a href="https://github.com/louisbove84/birds_of_play" class="github-link" target="_blank">
                    📚 View on GitHub
                </a>
            </div>

            <div style="margin-top: 40px; opacity: 0.8; font-size: 0.9rem;">
                <p>🚀 Deployed on Vercel | ⚡ Powered by Modern C++ & Machine Learning</p>
                <p>Latest commit: <code>ff85cd8</code> - DBSCAN clustering implementation</p>
            </div>
        </div>
    </body>
    </html>
    `);
}