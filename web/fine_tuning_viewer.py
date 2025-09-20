"""
Fine-tuning web interface for bird classification.
Provides Flask web server for manual labeling interface.
"""

from flask import Flask, render_template, request, jsonify, send_file
from flask_cors import CORS
from pathlib import Path
import sys

# Add project paths
project_root = Path(__file__).resolve().parent.parent
ml_src_path = project_root / "src" / "unsupervised_ml"
sys.path.insert(0, str(project_root))
sys.path.insert(0, str(ml_src_path))

# Import the fine-tuning service
from fine_tuning_service import FineTuningService

app = Flask(__name__, template_folder='templates')
CORS(app)

# Initialize the fine-tuning service
fine_tuning_service = FineTuningService()

@app.route('/')
def index():
    """Main fine-tuning interface."""
    return render_template('fine_tuning.html')

@app.route('/api/next-prediction')
def api_next_prediction():
    """Get the next prediction that needs labeling."""
    try:
        predictions = fine_tuning_service.get_uncertain_predictions()
        
        if not predictions:
            return jsonify({
                'success': True,
                'prediction': None,
                'message': 'No more predictions to label!',
                'remaining': 0
            })
        
        # Get the first (most uncertain) prediction
        next_pred = predictions[0]
        remaining = len(predictions) - 1
        
        print(f"Next prediction: {next_pred['image_path'].split('/')[-1]} with {next_pred['max_confidence']:.3f} confidence")
        
        return jsonify({
            'success': True,
            'prediction': next_pred,
            'remaining': remaining
        })
        
    except Exception as e:
        print(f"Error getting next prediction: {e}")
        return jsonify({
            'success': False,
            'error': str(e),
            'prediction': None,
            'remaining': 0
        })

@app.route('/api/stats')
def api_stats():
    """Get current labeling statistics."""
    try:
        return jsonify(fine_tuning_service.get_stats())
    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/api/record-correction', methods=['POST'])
def api_record_correction():
    """Record a user correction."""
    try:
        correction = request.json
        result = fine_tuning_service.record_correction(correction)
        return jsonify(result)
        
    except Exception as e:
        return jsonify({'success': False, 'error': str(e)})

@app.route('/api/bird-image/<filename>')
def serve_bird_image(filename):
    """Serve bird images for the interface."""
    try:
        # Look for the image in the objects directory
        objects_dir = project_root / "data" / "objects"
        image_path = objects_dir / filename
        
        if image_path.exists():
            return send_file(str(image_path), mimetype='image/jpeg')
        else:
            # Return a placeholder if image not found
            print(f"Image not found: {filename}")
            return jsonify({'error': 'Image not found'}), 404
            
    except Exception as e:
        print(f"Error serving image {filename}: {e}")
        return jsonify({'error': str(e)}), 500

@app.route('/api/retrain-with-corrections', methods=['POST'])
def api_retrain_with_corrections():
    """Retrain the model using user corrections."""
    try:
        result = fine_tuning_service.retrain_with_corrections()
        return jsonify(result)
            
    except Exception as e:
        return jsonify({
            'success': False,
            'error': str(e)
        })

# Removed add_new_class endpoint - focusing on core features

@app.route('/api/model-performance')
def api_model_performance():
    """Get current and previous model performance metrics."""
    try:
        result = fine_tuning_service.get_model_performance()
        
        if 'error' in result:
            return jsonify(result), 400
            
        return jsonify(result)
    except Exception as e:
        return jsonify({'error': str(e)}), 500

@app.route('/api/debug-predictions')
def api_debug_predictions():
    """Debug endpoint to see all predictions regardless of confidence."""
    try:
        # Get a few predictions for debugging
        predictions = fine_tuning_service.get_uncertain_predictions(max_samples=5)
        
        debug_info = {
            'total_predictions': len(predictions),
            'predictions': predictions[:3] if predictions else [],  # Show first 3 for brevity
            'config_info': 'Using fine-tuning service'
        }
        
        return jsonify(debug_info)
    except Exception as e:
        return jsonify({'error': str(e)}), 500

if __name__ == '__main__':
    print("🧠 Starting Bird Fine-Tuning Web Interface on http://localhost:3003")
    print("📊 Navigate to http://localhost:3003 to label low-confidence predictions")
    print(f"📋 Loaded {len(fine_tuning_service.labeled_corrections)} previous corrections")
    print("")
    print("🌐 Navigation:")
    print("   📹 Motion Detection: http://localhost:3000")
    print("   🎯 Object Detection: http://localhost:3001")
    print("   🔬 Bird Clustering: http://localhost:3002")
    print("   🧠 Fine-Tuning: http://localhost:3003")
    print("")
    print("Make sure you have:")
    print("1. Trained the supervised model: python train_supervised_pipeline.py")
    print("2. Model file exists: src/unsupervised_ml/trained_bird_classifier.pth")
    
    try:
        app.run(host='0.0.0.0', port=3003, debug=False)
    except Exception as e:
        print(f"Error starting server: {e}")
        print(f"Make sure port 3003 is available")