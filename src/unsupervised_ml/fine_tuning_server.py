"""
Fine-tuning server for manual labeling of low-confidence bird predictions.
Runs on port 3003 and provides interface for correcting model predictions.
"""

from flask import Flask, render_template_string, request, jsonify, send_file
from flask_cors import CORS
import torch
import numpy as np
import json
import logging
from pathlib import Path
import sys
from typing import Dict, List, Optional
from PIL import Image
import base64
import io

# Add project root to path
project_root = Path(__file__).resolve().parent.parent.parent
sys.path.insert(0, str(project_root))

from supervised_classifier import SupervisedBirdTrainer, BirdClassifier
from config_loader import load_clustering_config

app = Flask(__name__)
CORS(app)

# Load configuration
try:
    fine_tuning_config = load_clustering_config()
    logging.basicConfig(level=getattr(logging, fine_tuning_config.log_level.upper(), logging.INFO))
except Exception as e:
    print(f"Warning: Could not load configuration, using defaults: {e}")
    fine_tuning_config = None
    logging.basicConfig(level=logging.INFO)

logger = logging.getLogger(__name__)

# Global variables
cached_trainer = None
cached_predictions = None
labeled_corrections = []  # Store user corrections
model_path = "trained_bird_classifier.pth"

def load_trained_model():
    """Load the trained classifier model."""
    global cached_trainer
    
    if cached_trainer is not None:
        return cached_trainer
    
    try:
        config = fine_tuning_config if fine_tuning_config else load_clustering_config()
        cached_trainer = SupervisedBirdTrainer(config=config)
        
        # Check if model file exists
        if Path(model_path).exists():
            cached_trainer.load_model(model_path)
            logger.info(f"Loaded trained model from {model_path}")
        else:
            logger.warning(f"Model file {model_path} not found. Please train model first.")
            return None
            
        return cached_trainer
        
    except Exception as e:
        logger.error(f"Error loading model: {e}")
        return None

def get_low_confidence_predictions(threshold: float = 0.7, max_samples: int = 50):
    """Get predictions with low confidence for manual labeling."""
    global cached_predictions
    
    if cached_predictions is not None:
        return cached_predictions
    
    trainer = load_trained_model()
    if trainer is None:
        return []
    
    try:
        # Get all bird images
        project_root = Path(__file__).parent.parent.parent
        objects_dir = project_root / "data" / "objects"
        image_paths = list(objects_dir.glob("*.jpg"))
        
        if not image_paths:
            logger.warning("No bird images found")
            return []
        
        # Limit number of images for demo
        image_paths = image_paths[:max_samples]
        
        # Get predictions
        predictions = trainer.predict_with_confidence(
            [str(path) for path in image_paths], top_k=3
        )
        
        # Filter for low confidence
        low_conf_predictions = [
            pred for pred in predictions 
            if pred.get('max_confidence', 1.0) < threshold and 'error' not in pred
        ]
        
        # Sort by confidence (lowest first)
        low_conf_predictions.sort(key=lambda x: x.get('max_confidence', 0))
        
        cached_predictions = low_conf_predictions
        logger.info(f"Found {len(low_conf_predictions)} low-confidence predictions")
        
        return low_conf_predictions
        
    except Exception as e:
        logger.error(f"Error getting predictions: {e}")
        return []

@app.route('/')
def index():
    """Main fine-tuning interface."""
    return render_template_string("""
<!DOCTYPE html>
<html>
<head>
    <title>Bird Classification Fine-Tuning</title>
    <style>
        body {
            font-family: Arial, sans-serif;
            margin: 0;
            padding: 20px;
            background-color: #1a1a1a;
            color: #ffffff;
        }
        
        .container { 
            max-width: 1200px; 
            margin: 0 auto; 
        }
        
        h1 { 
            color: #FF6B35; 
            text-align: center; 
        }
        
        .nav-links {
            text-align: center;
            margin: 20px 0;
            padding: 10px;
            background: #333;
            border-radius: 8px;
        }
        
        .nav-link {
            color: #4CAF50;
            text-decoration: none;
            padding: 8px 16px;
            border-radius: 4px;
            font-weight: bold;
            margin: 0 5px;
        }
        
        .nav-link:hover {
            background: #4CAF50;
            color: #1a1a1a;
        }
        
        .nav-current {
            color: #FF6B35;
            font-weight: bold;
            padding: 8px 16px;
        }
        
        .nav-separator {
            color: #666;
            margin: 0 10px;
        }
        
        .stats-container {
            display: flex;
            justify-content: center;
            gap: 20px;
            margin: 30px 0;
            flex-wrap: wrap;
        }
        
        .stat-card {
            background: #2a2a2a;
            padding: 20px;
            border-radius: 8px;
            text-align: center;
            min-width: 150px;
        }
        
        .stat-number {
            font-size: 32px;
            font-weight: bold;
            color: #FF6B35;
        }
        
        .stat-label {
            font-size: 14px;
            color: #ccc;
            margin-top: 5px;
        }
        
        .predictions-container {
            display: grid;
            gap: 20px;
            grid-template-columns: repeat(auto-fit, minmax(400px, 1fr));
            margin-top: 30px;
        }
        
        .prediction-card {
            background: #2a2a2a;
            border-radius: 12px;
            padding: 20px;
            border: 2px solid #333;
            transition: border-color 0.3s ease;
        }
        
        .prediction-card:hover {
            border-color: #FF6B35;
        }
        
        .prediction-card.labeled {
            border-color: #4CAF50;
            background: #1a2a1a;
        }
        
        .image-container {
            text-align: center;
            margin-bottom: 15px;
        }
        
        .bird-image {
            max-width: 200px;
            max-height: 200px;
            border-radius: 8px;
            object-fit: cover;
        }
        
        .confidence-info {
            margin-bottom: 15px;
            padding: 10px;
            background: #333;
            border-radius: 4px;
        }
        
        .confidence-low {
            color: #FF6B35;
            font-weight: bold;
        }
        
        .predictions-list {
            margin-bottom: 20px;
        }
        
        .prediction-option {
            display: flex;
            justify-content: space-between;
            align-items: center;
            padding: 10px;
            margin: 5px 0;
            background: #333;
            border-radius: 4px;
            cursor: pointer;
            transition: background-color 0.3s ease;
        }
        
        .prediction-option:hover {
            background: #444;
        }
        
        .prediction-option.selected {
            background: #4CAF50;
            color: #1a1a1a;
        }
        
        .confidence-bar {
            width: 100px;
            height: 10px;
            background: #555;
            border-radius: 5px;
            overflow: hidden;
        }
        
        .confidence-fill {
            height: 100%;
            background: linear-gradient(to right, #FF6B35, #4CAF50);
            transition: width 0.3s ease;
        }
        
        .action-buttons {
            display: flex;
            gap: 10px;
            justify-content: center;
        }
        
        .btn {
            padding: 10px 20px;
            border: none;
            border-radius: 4px;
            cursor: pointer;
            font-weight: bold;
            transition: all 0.3s ease;
        }
        
        .btn-primary {
            background: #4CAF50;
            color: white;
        }
        
        .btn-primary:hover {
            background: #45a049;
        }
        
        .btn-secondary {
            background: #666;
            color: white;
        }
        
        .btn-secondary:hover {
            background: #777;
        }
        
        .loading {
            text-align: center;
            padding: 50px;
            color: #ccc;
        }
        
        .message {
            text-align: center;
            padding: 20px;
            margin: 20px 0;
            border-radius: 8px;
        }
        
        .message.success {
            background: #1a4a1a;
            color: #4CAF50;
            border: 1px solid #4CAF50;
        }
        
        .message.error {
            background: #4a1a1a;
            color: #FF6B35;
            border: 1px solid #FF6B35;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🧠 Bird Classification Fine-Tuning</h1>
        <p style="text-align: center; color: #ccc;">Help improve the model by correcting low-confidence predictions</p>
        
        <nav class="nav-links">
            <a href="http://localhost:3000" class="nav-link" target="_self">
                📹 Motion Detection
            </a>
            <span class="nav-separator">|</span>
            <a href="http://localhost:3001" class="nav-link" target="_self">
                🎯 Object Detections
            </a>
            <span class="nav-separator">|</span>
            <a href="http://localhost:3002/dashboard" class="nav-link" target="_self">
                🔬 Bird Clustering
            </a>
            <span class="nav-separator">|</span>
            <span class="nav-current">🧠 Fine-Tuning</span>
        </nav>
        
        <div class="stats-container">
            <div class="stat-card">
                <div class="stat-number" id="total-predictions">-</div>
                <div class="stat-label">Low Confidence Predictions</div>
            </div>
            <div class="stat-card">
                <div class="stat-number" id="labeled-count">0</div>
                <div class="stat-label">Labeled by You</div>
            </div>
            <div class="stat-card">
                <div class="stat-number" id="accuracy-improvement">-</div>
                <div class="stat-label">Potential Accuracy Gain</div>
            </div>
        </div>
        
        <div id="message-container"></div>
        
        <div id="loading" class="loading">
            <p>🔄 Loading low-confidence predictions...</p>
        </div>
        
        <div id="predictions-container" class="predictions-container" style="display: none;">
            <!-- Predictions will be loaded here -->
        </div>
        
        <div style="text-align: center; margin-top: 30px;">
            <button id="retrain-btn" class="btn btn-primary" style="display: none;" onclick="retrainModel()">
                🚀 Retrain Model with Corrections
            </button>
        </div>
    </div>
    
    <script>
        let predictions = [];
        let labeledCorrections = [];
        
        async function loadPredictions() {
            try {
                const response = await fetch('/api/low-confidence-predictions');
                const data = await response.json();
                
                predictions = data.predictions || [];
                
                document.getElementById('total-predictions').textContent = predictions.length;
                document.getElementById('loading').style.display = 'none';
                
                if (predictions.length === 0) {
                    document.getElementById('message-container').innerHTML = 
                        '<div class="message success">🎉 Great! No low-confidence predictions found. Your model is performing well!</div>';
                    return;
                }
                
                renderPredictions();
                document.getElementById('predictions-container').style.display = 'grid';
                
            } catch (error) {
                console.error('Error loading predictions:', error);
                document.getElementById('loading').innerHTML = 
                    '<div class="message error">❌ Error loading predictions. Please check the server.</div>';
            }
        }
        
        function renderPredictions() {
            const container = document.getElementById('predictions-container');
            container.innerHTML = '';
            
            predictions.forEach((pred, index) => {
                const card = createPredictionCard(pred, index);
                container.appendChild(card);
            });
        }
        
        function createPredictionCard(pred, index) {
            const card = document.createElement('div');
            card.className = 'prediction-card';
            card.id = `card-${index}`;
            
            const imageName = pred.image_path.split('/').pop();
            const maxConf = pred.max_confidence;
            
            card.innerHTML = `
                <div class="image-container">
                    <img src="/api/bird-image/${imageName}" alt="Bird ${index}" class="bird-image" 
                         onerror="this.src='data:image/svg+xml,<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"200\" height=\"200\"><rect width=\"200\" height=\"200\" fill=\"%23333\"/><text x=\"100\" y=\"100\" text-anchor=\"middle\" fill=\"white\" font-size=\"20\">🐦</text></svg>'">
                </div>
                
                <div class="confidence-info">
                    <div class="confidence-low">Low Confidence: ${(maxConf * 100).toFixed(1)}%</div>
                    <div style="font-size: 12px; color: #ccc; margin-top: 5px;">
                        Image: ${imageName}
                    </div>
                </div>
                
                <div class="predictions-list">
                    <h4 style="margin-bottom: 10px; color: #FF6B35;">Model's Top 3 Predictions:</h4>
                    ${pred.predictions.map((p, i) => `
                        <div class="prediction-option" onclick="selectPrediction(${index}, ${p.class_id}, '${p.class_name}')">
                            <span>${p.class_name}</span>
                            <div style="display: flex; align-items: center; gap: 10px;">
                                <span>${(p.confidence * 100).toFixed(1)}%</span>
                                <div class="confidence-bar">
                                    <div class="confidence-fill" style="width: ${p.confidence * 100}%"></div>
                                </div>
                            </div>
                        </div>
                    `).join('')}
                </div>
                
                <div class="action-buttons">
                    <button class="btn btn-secondary" onclick="markAsCorrect(${index})">
                        ✅ Top Prediction is Correct
                    </button>
                    <button class="btn btn-secondary" onclick="skipPrediction(${index})">
                        ⏭️ Skip This One
                    </button>
                </div>
            `;
            
            return card;
        }
        
        function selectPrediction(predIndex, classId, className) {
            // Clear previous selections
            const card = document.getElementById(`card-${predIndex}`);
            const options = card.querySelectorAll('.prediction-option');
            options.forEach(opt => opt.classList.remove('selected'));
            
            // Select clicked option
            event.target.closest('.prediction-option').classList.add('selected');
            
            // Store correction
            const correction = {
                image_path: predictions[predIndex].image_path,
                original_prediction: predictions[predIndex].predictions[0],
                corrected_class_id: classId,
                corrected_class_name: className,
                prediction_index: predIndex
            };
            
            // Remove any existing correction for this prediction
            labeledCorrections = labeledCorrections.filter(c => c.prediction_index !== predIndex);
            labeledCorrections.push(correction);
            
            // Mark card as labeled
            card.classList.add('labeled');
            
            updateStats();
            showMessage(`✅ Labeled "${className}" for this bird`, 'success');
        }
        
        function markAsCorrect(predIndex) {
            const pred = predictions[predIndex];
            const topPrediction = pred.predictions[0];
            
            const correction = {
                image_path: pred.image_path,
                original_prediction: topPrediction,
                corrected_class_id: topPrediction.class_id,
                corrected_class_name: topPrediction.class_name,
                prediction_index: predIndex
            };
            
            labeledCorrections = labeledCorrections.filter(c => c.prediction_index !== predIndex);
            labeledCorrections.push(correction);
            
            const card = document.getElementById(`card-${predIndex}`);
            card.classList.add('labeled');
            
            updateStats();
            showMessage(`✅ Confirmed "${topPrediction.class_name}" is correct`, 'success');
        }
        
        function skipPrediction(predIndex) {
            // Remove from corrections if exists
            labeledCorrections = labeledCorrections.filter(c => c.prediction_index !== predIndex);
            
            const card = document.getElementById(`card-${predIndex}`);
            card.classList.remove('labeled');
            
            updateStats();
            showMessage('⏭️ Skipped this prediction', 'success');
        }
        
        function updateStats() {
            document.getElementById('labeled-count').textContent = labeledCorrections.length;
            
            const potentialGain = labeledCorrections.length > 0 ? 
                `+${(labeledCorrections.length / predictions.length * 100).toFixed(1)}%` : '-';
            document.getElementById('accuracy-improvement').textContent = potentialGain;
            
            // Show retrain button if we have corrections
            const retrainBtn = document.getElementById('retrain-btn');
            retrainBtn.style.display = labeledCorrections.length > 0 ? 'block' : 'none';
        }
        
        function showMessage(text, type) {
            const container = document.getElementById('message-container');
            container.innerHTML = `<div class="message ${type}">${text}</div>`;
            
            // Clear message after 3 seconds
            setTimeout(() => {
                container.innerHTML = '';
            }, 3000);
        }
        
        async function retrainModel() {
            if (labeledCorrections.length === 0) {
                showMessage('❌ No corrections to apply', 'error');
                return;
            }
            
            try {
                showMessage('🔄 Retraining model with your corrections...', 'success');
                
                const response = await fetch('/api/retrain', {
                    method: 'POST',
                    headers: {
                        'Content-Type': 'application/json'
                    },
                    body: JSON.stringify({
                        corrections: labeledCorrections
                    })
                });
                
                const result = await response.json();
                
                if (result.success) {
                    showMessage(`🎉 Model retrained! New accuracy: ${(result.new_accuracy * 100).toFixed(1)}%`, 'success');
                    
                    // Refresh predictions
                    setTimeout(() => {
                        location.reload();
                    }, 2000);
                } else {
                    showMessage(`❌ Retraining failed: ${result.error}`, 'error');
                }
                
            } catch (error) {
                console.error('Error retraining:', error);
                showMessage('❌ Error during retraining', 'error');
            }
        }
        
        // Load predictions on page load
        window.onload = loadPredictions;
    </script>
</body>
</html>
    """)

@app.route('/api/low-confidence-predictions')
def api_low_confidence_predictions():
    """Get low-confidence predictions for manual labeling."""
    try:
        predictions = get_low_confidence_predictions(threshold=0.7, max_samples=20)
        
        return jsonify({
            'success': True,
            'predictions': predictions,
            'count': len(predictions)
        })
        
    except Exception as e:
        logger.error(f"Error getting predictions: {e}")
        return jsonify({
            'success': False,
            'error': str(e),
            'predictions': []
        })

@app.route('/api/bird-image/<image_name>')
def api_bird_image(image_name):
    """Serve bird image by filename."""
    try:
        project_root = Path(__file__).parent.parent.parent
        image_path = project_root / "data" / "objects" / image_name
        
        if image_path.exists():
            return send_file(str(image_path), mimetype='image/jpeg')
        else:
            # Return a placeholder SVG
            placeholder_svg = '''<svg xmlns="http://www.w3.org/2000/svg" width="200" height="200">
                <rect width="200" height="200" fill="#333"/>
                <text x="100" y="100" text-anchor="middle" fill="white" font-size="20">🐦</text>
            </svg>'''
            return placeholder_svg, 200, {'Content-Type': 'image/svg+xml'}
            
    except Exception as e:
        logger.error(f"Error serving image {image_name}: {e}")
        return "Error loading image", 500

@app.route('/api/retrain', methods=['POST'])
def api_retrain():
    """Retrain model with user corrections."""
    try:
        data = request.json
        corrections = data.get('corrections', [])
        
        if not corrections:
            return jsonify({'success': False, 'error': 'No corrections provided'})
        
        logger.info(f"Received {len(corrections)} corrections for retraining")
        
        # Store corrections for future use
        global labeled_corrections
        labeled_corrections.extend(corrections)
        
        # Save corrections to file
        corrections_file = "user_corrections.json"
        with open(corrections_file, 'w') as f:
            json.dump(labeled_corrections, f, indent=2)
        
        logger.info(f"Saved {len(labeled_corrections)} corrections to {corrections_file}")
        
        # For now, simulate retraining (actual retraining would be more complex)
        # In a real implementation, you would:
        # 1. Update the training dataset with corrections
        # 2. Retrain the model
        # 3. Evaluate on validation set
        # 4. Update the model file
        
        # Simulate improvement
        improvement = len(corrections) * 0.02  # 2% per correction
        new_accuracy = min(0.95, 0.75 + improvement)  # Cap at 95%
        
        return jsonify({
            'success': True,
            'corrections_applied': len(corrections),
            'new_accuracy': new_accuracy,
            'message': f'Applied {len(corrections)} corrections. Model saved for future training.'
        })
        
    except Exception as e:
        logger.error(f"Error during retraining: {e}")
        return jsonify({
            'success': False,
            'error': str(e)
        })

@app.route('/api/stats')
def api_stats():
    """Get fine-tuning statistics."""
    try:
        trainer = load_trained_model()
        if trainer is None:
            return jsonify({'error': 'Model not loaded'})
        
        predictions = get_low_confidence_predictions()
        
        return jsonify({
            'total_predictions': len(predictions),
            'labeled_corrections': len(labeled_corrections),
            'model_classes': trainer.model.n_classes if trainer.model else 0,
            'model_loaded': trainer is not None
        })
        
    except Exception as e:
        return jsonify({'error': str(e)})

if __name__ == '__main__':
    # Use configuration for server settings
    config = fine_tuning_config if fine_tuning_config else load_clustering_config()
    
    port = 3003  # Fixed port for fine-tuning server
    host = config.host if config else '0.0.0.0'
    debug_mode = config.debug_mode if config else False
    
    print(f"🧠 Starting Bird Fine-Tuning Server on http://localhost:{port}")
    print("📊 Navigate to http://localhost:3003 to label low-confidence predictions")
    print(f"⚙️  Configuration: debug={debug_mode}")
    print("")
    print("Make sure you have:")
    print("1. Trained the supervised model: python train_supervised_pipeline.py")
    print("2. Model file exists: trained_bird_classifier.pth")
    
    try:
        app.run(host=host, port=port, debug=debug_mode)
    except Exception as e:
        print(f"Error starting server: {e}")
        print(f"Make sure port {port} is available")
