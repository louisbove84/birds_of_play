// Bird Clustering Viewer (Port 3002 equivalent)
const express = require('express');
const app = express();

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

app.get('/', (req, res) => {
    res.send(`
<!DOCTYPE html>
<html>
<head>
    <title>🦅 Birds of Play - Bird Clustering Analysis</title>
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
        .stat-label {
            font-size: 1.1rem;
            opacity: 0.9;
        }
        .method-comparison {
            background: rgba(255, 255, 255, 0.1);
            border-radius: 15px;
            padding: 25px;
            margin: 30px 0;
        }
        .method-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
            gap: 15px;
            margin: 20px 0;
        }
        .method-card {
            background: rgba(0, 0, 0, 0.3);
            border-radius: 10px;
            padding: 15px;
            border: 1px solid rgba(255, 255, 255, 0.2);
            transition: all 0.3s ease;
        }
        .method-card.best {
            border-color: #4CAF50;
            background: rgba(76, 175, 80, 0.1);
        }
        .method-card:hover {
            transform: translateY(-3px);
        }
        .method-name {
            font-weight: bold;
            color: #FFD700;
            margin-bottom: 8px;
        }
        .method-card.best .method-name {
            color: #4CAF50;
        }
        .cluster-grid { 
            display: grid; 
            grid-template-columns: repeat(auto-fill, minmax(300px, 1fr)); 
            gap: 25px; 
            margin: 30px 0;
        }
        .cluster-card {
            background: rgba(255, 255, 255, 0.1);
            border-radius: 15px;
            padding: 25px;
            border: 1px solid rgba(255, 255, 255, 0.2);
            transition: all 0.3s ease;
        }
        .cluster-card:hover {
            transform: translateY(-5px);
            border-color: #E91E63;
        }
        .cluster-header {
            display: flex;
            justify-content: space-between;
            align-items: center;
            margin-bottom: 20px;
            padding-bottom: 15px;
            border-bottom: 1px solid rgba(255, 255, 255, 0.2);
        }
        .cluster-name {
            font-weight: bold;
            color: #E91E63;
            font-size: 1.2rem;
        }
        .cluster-count {
            background: linear-gradient(45deg, #E91E63, #C2185B);
            color: white;
            padding: 5px 12px;
            border-radius: 15px;
            font-size: 0.9rem;
            font-weight: bold;
        }
        .cluster-info {
            margin: 15px 0;
        }
        .confidence-bar {
            background: rgba(255, 255, 255, 0.2);
            border-radius: 10px;
            height: 8px;
            margin: 10px 0;
            overflow: hidden;
        }
        .confidence-fill {
            height: 100%;
            background: linear-gradient(45deg, #4CAF50, #45a049);
            border-radius: 10px;
            transition: width 0.3s ease;
        }
        .characteristics {
            background: rgba(156, 39, 176, 0.1);
            border: 1px solid rgba(156, 39, 176, 0.3);
            border-radius: 8px;
            padding: 12px;
            margin: 15px 0;
            font-style: italic;
            font-size: 0.95rem;
        }
        .objects-list {
            background: rgba(0, 0, 0, 0.3);
            border-radius: 8px;
            padding: 12px;
            margin: 15px 0;
        }
        .objects-count {
            font-size: 0.9rem;
            opacity: 0.8;
            margin-bottom: 8px;
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
        @media (max-width: 768px) {
            .nav-links {
                flex-direction: column;
                align-items: center;
            }
            .cluster-grid {
                grid-template-columns: 1fr;
            }
            .method-grid {
                grid-template-columns: 1fr;
            }
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🔬 Bird Clustering Analysis</h1>
            <p class="subtitle">Hierarchical Clustering & Species Identification</p>
            
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

        <div class="method-comparison">
            <h3 style="color: #FFD700; text-align: center; margin-bottom: 20px;">📊 Clustering Method Comparison</h3>
            <div class="method-grid">
                ${Object.entries(mockClusteringData.methods).map(([method, data]) => `
                    <div class="method-card ${method === mockClusteringData.bestMethod ? 'best' : ''}">
                        <div class="method-name">
                            ${method.replace('_', ' ').toUpperCase()}
                            ${method === mockClusteringData.bestMethod ? ' ⭐' : ''}
                        </div>
                        <div style="font-size: 0.9rem; margin: 5px 0;">
                            Clusters: <strong>${data.clusters}</strong>
                        </div>
                        <div style="font-size: 0.9rem; margin: 5px 0;">
                            Silhouette: <strong>${data.silhouette}</strong>
                        </div>
                    </div>
                `).join('')}
            </div>
        </div>

        <h2 style="color: #E91E63; text-align: center; margin: 40px 0 30px 0;">🐦 Identified Species Clusters</h2>
        
        <div class="cluster-grid">
            ${mockClusteringData.clusters.map(cluster => `
                <div class="cluster-card">
                    <div class="cluster-header">
                        <div class="cluster-name">${cluster.name}</div>
                        <div class="cluster-count">${cluster.count} birds</div>
                    </div>
                    
                    <div class="cluster-info">
                        <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 5px;">
                            <span style="font-size: 0.9rem;">Confidence</span>
                            <span style="font-weight: bold; color: #4CAF50;">${Math.round(cluster.confidence * 100)}%</span>
                        </div>
                        <div class="confidence-bar">
                            <div class="confidence-fill" style="width: ${cluster.confidence * 100}%"></div>
                        </div>
                    </div>
                    
                    <div class="characteristics">
                        ${cluster.characteristics}
                    </div>
                    
                    <div class="objects-list">
                        <div class="objects-count">Objects in cluster: ${cluster.count}</div>
                        <div style="font-family: 'Courier New', monospace; font-size: 0.85rem; opacity: 0.8;">
                            ${cluster.objects.slice(0, 3).join(', ')}${cluster.objects.length > 3 ? ` +${cluster.objects.length - 3} more` : ''}
                        </div>
                    </div>
                </div>
            `).join('')}
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
});

app.get('/api/clusters', (req, res) => {
    res.json({
        status: 'success',
        clustering: mockClusteringData,
        featureExtraction: {
            model: 'ResNet50',
            dimensions: 2048,
            device: 'cpu'
        },
        bestMethod: mockClusteringData.bestMethod
    });
});

module.exports = app;
