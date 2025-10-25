// Fine-Tuning Interface - Vercel Serverless Function
export default function handler(req, res) {
    // Set CORS headers
    res.setHeader('Access-Control-Allow-Origin', '*');
    res.setHeader('Access-Control-Allow-Methods', 'GET, POST, OPTIONS');
    res.setHeader('Access-Control-Allow-Headers', 'Content-Type');
    
    if (req.method === 'OPTIONS') {
        res.status(200).end();
        return;
    }

    // Mock fine-tuning data
    const mockFineTuningData = {
        modelStatus: {
            trained: true,
            accuracy: 0.80,
            meanConfidence: 0.945,
            epochs: 8,
            lastTrained: "2025-01-25T03:29:37.000Z"
        },
        corrections: [
            {
                id: "correction_001",
                objectId: "obj_015",
                originalPrediction: "Species Cluster 2",
                userCorrection: "Species Cluster 1", 
                confidence: 0.72,
                timestamp: "2025-01-25T03:30:15.000Z"
            },
            {
                id: "correction_002",
                objectId: "obj_023",
                originalPrediction: "Species Cluster 1",
                userCorrection: "Species Cluster 3",
                confidence: 0.68,
                timestamp: "2025-01-25T03:30:45.000Z"
            }
        ],
        pendingReview: [
            {
                objectId: "obj_025",
                prediction: "Species Cluster 4",
                confidence: 0.78,
                needsReview: true,
                reason: "Low confidence prediction"
            },
            {
                objectId: "obj_012", 
                prediction: "Species Cluster 2",
                confidence: 0.65,
                needsReview: true,
                reason: "Borderline confidence"
            }
        ]
    };

    if (req.url === '/api/model-status') {
        res.status(200).json({
            status: 'success',
            model: mockFineTuningData.modelStatus,
            corrections: mockFineTuningData.corrections,
            pendingReview: mockFineTuningData.pendingReview
        });
        return;
    }

    // Main fine-tuning interface HTML
    res.status(200).send(`
<!DOCTYPE html>
<html>
<head>
    <title>🦅 Birds of Play - Fine-Tuning Interface</title>
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
            color: #FF9800; 
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
            background: rgba(255, 152, 0, 0.2);
            color: #FF9800;
            padding: 10px 20px;
            border-radius: 25px;
            text-decoration: none;
            font-weight: bold;
            transition: all 0.3s ease;
            border: 2px solid rgba(255, 152, 0, 0.3);
        }
        .nav-link:hover, .nav-link.active {
            background: #FF9800;
            color: white;
            transform: translateY(-2px);
            box-shadow: 0 5px 15px rgba(255, 152, 0, 0.4);
        }
        .model-status {
            background: rgba(76, 175, 80, 0.1);
            border: 1px solid rgba(76, 175, 80, 0.3);
            border-radius: 15px;
            padding: 25px;
            margin: 30px 0;
        }
        .status-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 20px;
            margin: 20px 0;
        }
        .status-item {
            background: rgba(0, 0, 0, 0.3);
            border-radius: 10px;
            padding: 15px;
            text-align: center;
        }
        .status-value {
            font-size: 1.8rem;
            font-weight: bold;
            color: #4CAF50;
            margin-bottom: 5px;
        }
        .demo-notice {
            background: rgba(255, 152, 0, 0.1);
            border: 1px solid rgba(255, 152, 0, 0.3);
            border-radius: 10px;
            padding: 20px;
            margin: 20px 0;
            text-align: center;
        }
        .demo-notice h3 {
            color: #FF9800;
            margin: 0 0 10px 0;
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🧠 Fine-Tuning Interface</h1>
            <p>Human-in-the-Loop Model Improvement</p>
            
            <div class="nav-links">
                <a href="/api/motion" class="nav-link">📹 Motion Detection</a>
                <a href="/api/objects" class="nav-link">🎯 Object Detection</a>
                <a href="/api/clustering" class="nav-link">🔬 Bird Clustering</a>
                <a href="/api/finetuning" class="nav-link active">🧠 Fine-Tuning</a>
                <a href="/" class="nav-link">🏠 Home</a>
            </div>
        </div>

        <div class="demo-notice">
            <h3>🧠 Supervised Learning with Human Feedback</h3>
            <p>The model was trained using clustering pseudo-labels and can be improved with human corrections. This interface would allow users to review and correct low-confidence predictions.</p>
        </div>

        <div class="model-status">
            <h3 style="color: #4CAF50; text-align: center; margin-bottom: 20px;">✅ Model Status</h3>
            <div class="status-grid">
                <div class="status-item">
                    <div class="status-value">${Math.round(mockFineTuningData.modelStatus.accuracy * 100)}%</div>
                    <div class="status-label">Test Accuracy</div>
                </div>
                <div class="status-item">
                    <div class="status-value">${Math.round(mockFineTuningData.modelStatus.meanConfidence * 100)}%</div>
                    <div class="status-label">Mean Confidence</div>
                </div>
                <div class="status-item">
                    <div class="status-value">${mockFineTuningData.modelStatus.epochs}</div>
                    <div class="status-label">Training Epochs</div>
                </div>
                <div class="status-item">
                    <div class="status-value">${mockFineTuningData.corrections.length}</div>
                    <div class="status-label">User Corrections</div>
                </div>
            </div>
            <div style="text-align: center; margin-top: 15px; font-size: 0.9rem; opacity: 0.8;">
                Last trained: ${new Date(mockFineTuningData.modelStatus.lastTrained).toLocaleString()}
            </div>
        </div>

        <div style="text-align: center; margin: 40px 0; padding: 30px; background: rgba(255, 255, 255, 0.1); border-radius: 15px;">
            <h3 style="color: #FF9800; margin-bottom: 15px;">🔄 Fine-Tuning Process</h3>
            <div style="display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 20px; margin-top: 20px;">
                <div style="background: rgba(156, 39, 176, 0.1); padding: 15px; border-radius: 10px;">
                    <strong style="color: #9C27B0;">1. Initial Training</strong><br>
                    <span style="opacity: 0.9; font-size: 0.9rem;">Clustering pseudo-labels</span>
                </div>
                <div style="background: rgba(33, 150, 243, 0.1); padding: 15px; border-radius: 10px;">
                    <strong style="color: #2196F3;">2. Human Review</strong><br>
                    <span style="opacity: 0.9; font-size: 0.9rem;">Low-confidence predictions</span>
                </div>
                <div style="background: rgba(255, 152, 0, 0.1); padding: 15px; border-radius: 10px;">
                    <strong style="color: #FF9800;">3. Corrections</strong><br>
                    <span style="opacity: 0.9; font-size: 0.9rem;">User feedback integration</span>
                </div>
                <div style="background: rgba(76, 175, 80, 0.1); padding: 15px; border-radius: 10px;">
                    <strong style="color: #4CAF50;">4. Retraining</strong><br>
                    <span style="opacity: 0.9; font-size: 0.9rem;">Improved accuracy</span>
                </div>
            </div>
        </div>
    </div>
</body>
</html>
    `);
}