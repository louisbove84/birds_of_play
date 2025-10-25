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
                characteristics: "Small songbirds, compact build"
            },
            {
                id: 1, 
                name: "Species Cluster 2",
                count: 19,
                confidence: 0.87,
                characteristics: "Medium-sized birds, varied postures"
            },
            {
                id: 2,
                name: "Species Cluster 3", 
                count: 2,
                confidence: 0.95,
                characteristics: "Larger birds, distinctive features"
            },
            {
                id: 3,
                name: "Species Cluster 4",
                count: 1,
                confidence: 0.78,
                characteristics: "Unique specimen, requires review"
            }
        ],
        bestMethod: "ward_permissive"
    };

    if (req.url === '/api/clusters') {
        res.status(200).json({
            status: 'success',
            clustering: mockClusteringData
        });
        return;
    }

    // Generate cluster cards HTML
    const clusterCards = mockClusteringData.clusters.map(cluster => `
        <div class="cluster-card">
            <div class="cluster-header">
                <div class="cluster-name">${cluster.name}</div>
                <div class="cluster-count">${cluster.count} birds</div>
            </div>
            
            <div class="cluster-info">
                <div class="confidence-label">Confidence: ${Math.round(cluster.confidence * 100)}%</div>
                <div class="confidence-bar">
                    <div class="confidence-fill" style="width: ${cluster.confidence * 100}%"></div>
                </div>
            </div>
            
            <div class="characteristics">
                ${cluster.characteristics}
            </div>
        </div>
    `).join('');

    // Main clustering viewer HTML
    res.status(200).send(`
<!DOCTYPE html>
<html>
<head>
    <title>🦅 Birds of Play - Bird Clustering Analysis</title>
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
        .cluster-grid { 
            display: grid; 
            grid-template-columns: repeat(auto-fill, minmax(300px, 1fr)); 
            gap: 20px; 
            margin: 20px 0;
        }
        .cluster-card {
            background: rgba(255, 255, 255, 0.1);
            border-radius: 8px;
            padding: 20px;
            border: 1px solid rgba(255, 255, 255, 0.2);
        }
        .cluster-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 15px;
            padding-bottom: 10px;
            border-bottom: 1px solid rgba(255, 255, 255, 0.2);
        }
        .cluster-name {
            font-weight: bold;
            color: #E91E63;
        }
        .cluster-count {
            background: #E91E63;
            color: white;
            padding: 4px 8px;
            border-radius: 4px;
            font-size: 0.9rem;
            font-weight: bold;
        }
        .cluster-info {
            margin: 15px 0;
        }
        .confidence-label {
            margin-bottom: 5px;
            font-weight: bold;
            color: #4CAF50;
        }
        .confidence-bar {
            background: rgba(255, 255, 255, 0.2);
            border-radius: 4px;
            height: 8px;
            overflow: hidden;
        }
        .confidence-fill {
            height: 100%;
            background: #4CAF50;
            border-radius: 4px;
        }
        .characteristics {
            background: rgba(156, 39, 176, 0.1);
            border: 1px solid rgba(156, 39, 176, 0.3);
            border-radius: 4px;
            padding: 10px;
            margin: 15px 0;
            font-style: italic;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🔬 Bird Clustering Analysis</h1>
        
        <div class="nav-links">
            <a href="/">🏠 Home</a>
            <a href="/api/motion">📹 Motion Detection</a>
            <a href="/api/objects">🎯 Object Detection</a>
            <a href="/api/clustering">🔬 Bird Clustering</a>
            <a href="/api/finetuning">🧠 Fine-Tuning</a>
        </div>

        <h2>Identified Species Clusters</h2>
        
        <div class="cluster-grid">
            ${clusterCards}
        </div>
    </div>
</body>
</html>
    `);
}