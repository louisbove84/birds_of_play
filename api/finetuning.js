// Fine-Tuning Interface (Port 3003 equivalent)
const express = require('express');
const app = express();

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

app.get('/', (req, res) => {
    res.send(`
<!DOCTYPE html>
<html>
<head>
    <title>🦅 Birds of Play - Fine-Tuning Interface</title>
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
        .status-label {
            font-size: 0.9rem;
            opacity: 0.8;
        }
        .corrections-section {
            background: rgba(255, 255, 255, 0.1);
            border-radius: 15px;
            padding: 25px;
            margin: 30px 0;
        }
        .correction-item {
            background: rgba(33, 150, 243, 0.1);
            border: 1px solid rgba(33, 150, 243, 0.3);
            border-radius: 10px;
            padding: 20px;
            margin: 15px 0;
            display: grid;
            grid-template-columns: 1fr auto;
            gap: 20px;
            align-items: center;
        }
        .correction-info {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(150px, 1fr));
            gap: 15px;
        }
        .info-group {
            background: rgba(0, 0, 0, 0.3);
            padding: 10px;
            border-radius: 5px;
        }
        .info-label {
            font-size: 0.8rem;
            opacity: 0.7;
            margin-bottom: 3px;
        }
        .info-value {
            font-weight: bold;
            color: #FFD700;
        }
        .correction-arrow {
            font-size: 1.5rem;
            color: #4CAF50;
            text-align: center;
        }
        .pending-section {
            background: rgba(255, 193, 7, 0.1);
            border: 1px solid rgba(255, 193, 7, 0.3);
            border-radius: 15px;
            padding: 25px;
            margin: 30px 0;
        }
        .pending-item {
            background: rgba(255, 152, 0, 0.1);
            border: 1px solid rgba(255, 152, 0, 0.3);
            border-radius: 10px;
            padding: 20px;
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
            border-radius: 10px;
            transition: width 0.3s ease;
        }
        .confidence-low { background: linear-gradient(45deg, #f44336, #d32f2f); }
        .confidence-medium { background: linear-gradient(45deg, #FF9800, #F57C00); }
        .confidence-high { background: linear-gradient(45deg, #4CAF50, #45a049); }
        .action-buttons {
            display: flex;
            gap: 10px;
            margin-top: 15px;
        }
        .btn {
            padding: 8px 16px;
            border: none;
            border-radius: 20px;
            font-weight: bold;
            cursor: pointer;
            transition: all 0.3s ease;
            font-size: 0.9rem;
        }
        .btn-correct {
            background: linear-gradient(45deg, #4CAF50, #45a049);
            color: white;
        }
        .btn-reject {
            background: linear-gradient(45deg, #f44336, #d32f2f);
            color: white;
        }
        .btn:hover {
            transform: scale(1.05);
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
        @media (max-width: 768px) {
            .nav-links {
                flex-direction: column;
                align-items: center;
            }
            .correction-item {
                grid-template-columns: 1fr;
            }
            .correction-info {
                grid-template-columns: 1fr;
            }
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🧠 Fine-Tuning Interface</h1>
            <p class="subtitle">Human-in-the-Loop Model Improvement</p>
            
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

        <div class="corrections-section">
            <h3 style="color: #2196F3; margin-bottom: 20px;">📝 User Corrections Applied</h3>
            ${mockFineTuningData.corrections.map(correction => `
                <div class="correction-item">
                    <div class="correction-info">
                        <div class="info-group">
                            <div class="info-label">Object ID</div>
                            <div class="info-value">${correction.objectId}</div>
                        </div>
                        <div class="info-group">
                            <div class="info-label">Original Prediction</div>
                            <div class="info-value">${correction.originalPrediction}</div>
                        </div>
                        <div class="info-group">
                            <div class="info-label">User Correction</div>
                            <div class="info-value" style="color: #4CAF50;">${correction.userCorrection}</div>
                        </div>
                        <div class="info-group">
                            <div class="info-label">Confidence</div>
                            <div class="info-value">${Math.round(correction.confidence * 100)}%</div>
                        </div>
                    </div>
                    <div class="correction-arrow">✅</div>
                </div>
            `).join('')}
        </div>

        <div class="pending-section">
            <h3 style="color: #FFC107; margin-bottom: 20px;">⚠️ Predictions Needing Review</h3>
            ${mockFineTuningData.pendingReview.map(item => `
                <div class="pending-item">
                    <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 15px;">
                        <div>
                            <strong style="color: #FFD700;">${item.objectId}</strong>
                            <span style="margin-left: 15px; opacity: 0.8;">Predicted: ${item.prediction}</span>
                        </div>
                        <div style="font-size: 0.9rem; opacity: 0.8;">${item.reason}</div>
                    </div>
                    
                    <div style="display: flex; justify-content: space-between; align-items: center; margin-bottom: 10px;">
                        <span style="font-size: 0.9rem;">Confidence</span>
                        <span style="font-weight: bold; color: ${item.confidence < 0.7 ? '#f44336' : item.confidence < 0.8 ? '#FF9800' : '#4CAF50'};">${Math.round(item.confidence * 100)}%</span>
                    </div>
                    <div class="confidence-bar">
                        <div class="confidence-fill ${item.confidence < 0.7 ? 'confidence-low' : item.confidence < 0.8 ? 'confidence-medium' : 'confidence-high'}" style="width: ${item.confidence * 100}%"></div>
                    </div>
                    
                    <div class="action-buttons">
                        <button class="btn btn-correct" onclick="alert('In a live system, this would open a correction interface')">
                            ✏️ Correct Prediction
                        </button>
                        <button class="btn btn-reject" onclick="alert('In a live system, this would mark as reviewed')">
                            ✅ Accept Prediction
                        </button>
                    </div>
                </div>
            `).join('')}
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
});

app.get('/api/model-status', (req, res) => {
    res.json({
        status: 'success',
        model: mockFineTuningData.modelStatus,
        corrections: mockFineTuningData.corrections,
        pendingReview: mockFineTuningData.pendingReview
    });
});

module.exports = app;
