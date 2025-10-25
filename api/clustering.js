// Bird Clustering Viewer - Vercel Serverless Function
export default function handler(req, res) {
    // Set CORS headers
    res.setHeader('Access-Control-Allow-Origin', '*');
    res.setHeader('Access-Control-Allow-Methods', 'GET, POST, OPTIONS');
    res.setHeader('Access-Control-Allow-Headers', 'Content-Type');
    
    if (req.method === 'OPTIONS') {
        res.status(200).end();
        return;
    }

    // Mock clustering data from our pipeline test
    const mockClusteringData = {
        totalObjects: 25,
        clusters: [
            {
                id: 0,
                name: "Species Cluster 1",
                count: 3,
                confidence: 0.92,
                characteristics: "Small songbirds, compact build",
                objects: ["obj_001", "obj_008", "obj_015"]
            },
            {
                id: 1, 
                name: "Species Cluster 2",
                count: 19,
                confidence: 0.87,
                characteristics: "Medium-sized birds, varied postures",
                objects: ["obj_002", "obj_003", "obj_004", "obj_005", "obj_006", "obj_007", "obj_009", "obj_010", "obj_011", "obj_012", "obj_013", "obj_014", "obj_016", "obj_017", "obj_018", "obj_019", "obj_020", "obj_021", "obj_022"]
            },
            {
                id: 2,
                name: "Species Cluster 3", 
                count: 2,
                confidence: 0.95,
                characteristics: "Larger birds, distinctive features",
                objects: ["obj_023", "obj_024"]
            },
            {
                id: 3,
                name: "Species Cluster 4",
                count: 1,
                confidence: 0.78,
                characteristics: "Unique specimen, requires review",
                objects: ["obj_025"]
            }
        ],
        methods: {
            ward_conservative: { clusters: 17, silhouette: 0.45 },
            ward_balanced: { clusters: 8, silhouette: 0.52 },
            ward_permissive: { clusters: 4, silhouette: 0.61 },
            average_conservative: { clusters: 19, silhouette: 0.42 },
            average_balanced: { clusters: 7, silhouette: 0.58 },
            average_permissive: { clusters: 1, silhouette: 0.12 }
        },
        bestMethod: "ward_permissive"
    };

    if (req.url === '/api/clusters') {
        res.status(200).json({
            status: 'success',
            clustering: mockClusteringData,
            featureExtraction: {
                model: 'ResNet50',
                dimensions: 2048,
                device: 'cpu'
            },
            bestMethod: mockClusteringData.bestMethod
        });
        return;
    }

    // Main clustering viewer HTML
    res.status(200).send(`
<!DOCTYPE html>
<html>
<head>
    <title>🦅 Birds of Play - Bird Clustering Analysis</title>
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
            color: #9C27B0; 
            font-size: 2.5rem;
            margin: 0 0 10px 0;
            text-shadow: 2px 2px 4px rgba(0,0,0,0.3);
        }
        .nav-links {
            display: flex;
            justify-content: center;
            gap: 15px;
            margin: 20px 0;
            flex-wrap: wrap;
        }
        .nav-link {
            background: rgba(156, 39, 176, 0.2);
            color: #9C27B0;
            padding: 10px 20px;
            border-radius: 25px;
            text-decoration: none;
            font-weight: bold;
            transition: all 0.3s ease;
            border: 2px solid rgba(156, 39, 176, 0.3);
        }
        .nav-link:hover, .nav-link.active {
            background: #9C27B0;
            color: white;
            transform: translateY(-2px);
            box-shadow: 0 5px 15px rgba(156, 39, 176, 0.4);
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
            color: #E91E63;
            margin-bottom: 10px;
        }
        .demo-notice {
            background: rgba(156, 39, 176, 0.1);
            border: 1px solid rgba(156, 39, 176, 0.3);
            border-radius: 10px;
            padding: 20px;
            margin: 20px 0;
            text-align: center;
        }
        .demo-notice h3 {
            color: #9C27B0;
            margin: 0 0 10px 0;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🔬 Bird Clustering Analysis</h1>
            <p>Hierarchical Clustering & Species Identification</p>
            
            <div class="nav-links">
                <a href="/api/motion" class="nav-link">📹 Motion Detection</a>
                <a href="/api/objects" class="nav-link">🎯 Object Detection</a>
                <a href="/api/clustering" class="nav-link active">🔬 Bird Clustering</a>
                <a href="/api/finetuning" class="nav-link">🧠 Fine-Tuning</a>
                <a href="/" class="nav-link">🏠 Home</a>
            </div>
        </div>

        <div class="demo-notice">
            <h3>🔬 ResNet50 Feature Extraction + Hierarchical Clustering</h3>
            <p>Using ResNet50 CNN to extract 2048-dimensional features from detected bird objects, then applying hierarchical clustering to identify species groups. Best method: Ward Permissive with 4 clusters.</p>
        </div>

        <div class="stats-grid">
            <div class="stat-card">
                <div class="stat-number">${mockClusteringData.totalObjects}</div>
                <div class="stat-label">Bird Objects</div>
            </div>
            <div class="stat-card">
                <div class="stat-number">${mockClusteringData.clusters.length}</div>
                <div class="stat-label">Species Clusters</div>
            </div>
            <div class="stat-card">
                <div class="stat-number">2048</div>
                <div class="stat-label">Feature Dimensions</div>
            </div>
            <div class="stat-card">
                <div class="stat-number">80%</div>
                <div class="stat-label">Model Accuracy</div>
            </div>
        </div>

        <div style="text-align: center; margin: 40px 0; padding: 30px; background: rgba(255, 255, 255, 0.1); border-radius: 15px;">
            <h3 style="color: #9C27B0; margin-bottom: 15px;">🧠 Machine Learning Pipeline</h3>
            <div style="display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 20px; margin-top: 20px;">
                <div style="background: rgba(33, 150, 243, 0.1); padding: 15px; border-radius: 10px;">
                    <strong style="color: #2196F3;">1. Feature Extraction</strong><br>
                    <span style="opacity: 0.9; font-size: 0.9rem;">ResNet50 CNN (2048D)</span>
                </div>
                <div style="background: rgba(156, 39, 176, 0.1); padding: 15px; border-radius: 10px;">
                    <strong style="color: #9C27B0;">2. Clustering</strong><br>
                    <span style="opacity: 0.9; font-size: 0.9rem;">Ward Hierarchical</span>
                </div>
                <div style="background: rgba(76, 175, 80, 0.1); padding: 15px; border-radius: 10px;">
                    <strong style="color: #4CAF50;">3. Species ID</strong><br>
                    <span style="opacity: 0.9; font-size: 0.9rem;">4 distinct groups</span>
                </div>
                <div style="background: rgba(255, 152, 0, 0.1); padding: 15px; border-radius: 10px;">
                    <strong style="color: #FF9800;">4. Supervised Training</strong><br>
                    <span style="opacity: 0.9; font-size: 0.9rem;">80% accuracy</span>
                </div>
            </div>
        </div>
    </div>
</body>
</html>
    `);
}