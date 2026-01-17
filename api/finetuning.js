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
                reason: "Low confidence prediction"
            },
            {
                objectId: "obj_012",
                prediction: "Species Cluster 2",
                confidence: 0.65,
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

    // Generate corrections HTML
    const correctionsHtml = mockFineTuningData.corrections.map(correction => `
        <div class="correction-item">
            <div class="correction-info">
                <div><strong>Object:</strong> ${correction.objectId}</div>
                <div><strong>Original:</strong> ${correction.originalPrediction}</div>
                <div><strong>Corrected:</strong> ${correction.userCorrection}</div>
                <div><strong>Confidence:</strong> ${Math.round(correction.confidence * 100)}%</div>
            </div>
        </div>
    `).join('');

    // Generate pending reviews HTML
    const pendingHtml = mockFineTuningData.pendingReview.map(item => `
        <div class="pending-item">
            <div class="pending-header">
                <strong>${item.objectId}</strong> - Predicted: ${item.prediction}
            </div>
            <div class="confidence-info">
                Confidence: ${Math.round(item.confidence * 100)}% - ${item.reason}
            </div>
            <div class="confidence-bar">
                <div class="confidence-fill" style="width: ${item.confidence * 100}%"></div>
            </div>
        </div>
    `).join('');

    // Main fine-tuning interface HTML
    res.status(200).send(`
<!DOCTYPE html>
<html>
<head>
    <title>🦅 Birds of Play - Fine-Tuning Interface</title>
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
        .model-status {
            background: rgba(76, 175, 80, 0.1);
            border: 1px solid rgba(76, 175, 80, 0.3);
            border-radius: 8px;
            padding: 20px;
            margin: 20px 0;
        }
        .status-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(150px, 1fr));
            gap: 15px;
            margin: 15px 0;
        }
        .status-item {
            background: rgba(0, 0, 0, 0.3);
            border-radius: 4px;
            padding: 10px;
            text-align: center;
        }
        .status-value {
            font-size: 1.5rem;
            font-weight: bold;
            color: #4CAF50;
            margin-bottom: 5px;
        }
        .status-label {
            font-size: 0.9rem;
            opacity: 0.8;
        }
        .corrections-section, .pending-section {
            background: rgba(255, 255, 255, 0.1);
            border-radius: 8px;
            padding: 20px;
            margin: 20px 0;
        }
        .correction-item, .pending-item {
            background: rgba(33, 150, 243, 0.1);
            border: 1px solid rgba(33, 150, 243, 0.3);
            border-radius: 4px;
            padding: 15px;
            margin: 10px 0;
        }
        .correction-info div {
            margin: 5px 0;
        }
        .pending-header {
            margin-bottom: 10px;
        }
        .confidence-info {
            margin: 10px 0;
            font-size: 0.9rem;
        }
        .confidence-bar {
            background: rgba(255, 255, 255, 0.2);
            border-radius: 4px;
            height: 8px;
            overflow: hidden;
            margin: 10px 0;
        }
        .confidence-fill {
            height: 100%;
            background: #FF9800;
            border-radius: 4px;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🧠 Fine-Tuning Interface</h1>
        
        <div class="nav-links">
            <a href="/" class="nav-link">🏠 Home</a>
            <a href="/api/motion" class="nav-link">📹 Motion Detection</a>
            <a href="/api/objects" class="nav-link">🎯 Object Detection</a>
            <a href="/api/clustering" class="nav-link">🔬 Bird Clustering</a>
            <a href="/api/finetuning" class="nav-link nav-link-active" aria-current="page">🧠 Fine-Tuning</a>
        </div>

        <div class="model-status">
            <h3>Model Status</h3>
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
            <div style="text-align: center; margin-top: 10px; font-size: 0.9rem; opacity: 0.8;">
                Last trained: ${new Date(mockFineTuningData.modelStatus.lastTrained).toLocaleString()}
            </div>
        </div>

        <div class="corrections-section">
            <h3>User Corrections Applied</h3>
            ${correctionsHtml}
        </div>

        <div class="pending-section">
            <h3>Predictions Needing Review</h3>
            ${pendingHtml}
        </div>
    </div>
</body>
</html>
    `);
}