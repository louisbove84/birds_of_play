"""
Web server for bird clustering visualization and analysis.
"""

import os
import sys
from pathlib import Path
from flask import Flask, jsonify, request, send_file
from flask_cors import CORS
import logging

# Add project root to path
project_root = Path(__file__).resolve().parent.parent.parent.parent
sys.path.insert(0, str(project_root))

try:
    from object_data_manager import ObjectDataManager
    from feature_extractor import FeatureExtractor, FeaturePipeline
    from bird_clusterer import BirdClusterer, ClusteringExperiment
    from cluster_visualizer import ClusterVisualizer
    from config_loader import load_clustering_config
except ImportError as e:
    print(f"Import error: {e}")
    print("Please ensure all dependencies are installed and run from the unsupervised_ml directory")

app = Flask(__name__)
CORS(app)

# Load configuration
try:
    clustering_config = load_clustering_config()
    logging.basicConfig(level=getattr(logging, clustering_config.log_level.upper(), logging.INFO))
except Exception as e:
    print(f"Warning: Could not load configuration, using defaults: {e}")
    clustering_config = None
    logging.basicConfig(level=logging.INFO)

logger = logging.getLogger(__name__)

# Global variables for caching
cached_features = None
cached_metadata = None
cached_clusterer = None
cached_visualizer = None
cached_config = clustering_config

def initialize_system():
    """Initialize the clustering system with data and models."""
    global cached_features, cached_metadata, cached_clusterer, cached_visualizer, cached_config
    
    logger.info("Initializing bird clustering system...")
    
    try:
        # Use configuration for all parameters
        config = cached_config if cached_config else load_clustering_config()
        
        with ObjectDataManager() as data_manager:
            feature_extractor = FeatureExtractor(model_name=config.model_name)
            pipeline = FeaturePipeline(data_manager, feature_extractor)
            
            features, metadata = pipeline.extract_all_features(min_confidence=config.min_confidence)
            
            if len(features) == 0:
                logger.warning("No bird objects found for clustering")
                return False
            
            cached_features = features
            cached_metadata = metadata
            
            logger.info(f"Loaded {len(features)} bird objects for clustering")
            
            experiment = ClusteringExperiment(features, metadata, config=config)
            results = experiment.run_all_methods()
            
            best_method, best_result = experiment.get_best_method()
            
            if best_result:
                cached_clusterer = best_result['clusterer']
                logger.info(f"Best clustering method: {best_method}")
                
                cached_visualizer = ClusterVisualizer(cached_clusterer, 
                                                    output_dir=str(project_root / "data" / "visualizations"))
                
                return True
            else:
                logger.error("No successful clustering methods")
                return False
                
    except Exception as e:
        logger.error(f"Failed to initialize system: {e}")
        return False

@app.route('/')
def index():
    """Main clustering page."""
    if cached_clusterer is None:
        return f"""
        <html>
        <head><title>Bird Clustering - Initializing</title></head>
        <body style="background-color: #1a1a1a; color: #ffffff; font-family: Arial, sans-serif; text-align: center; padding: 50px;">
            <h1>🔬 Bird Clustering System</h1>
            <p>System is initializing... Please wait.</p>
            <p><a href="/initialize" style="color: #4CAF50;">Click here to initialize</a></p>
        </body>
        </html>
        """
    
    # Redirect to dashboard as the main page
    return dashboard()

@app.route('/initialize')
def initialize():
    """Initialize the clustering system."""
    success = initialize_system()
    
    if success:
        return jsonify({'status': 'success', 'message': 'System initialized successfully'})
    else:
        return jsonify({'status': 'error', 'message': 'Failed to initialize system'})


@app.route('/dashboard')
def dashboard():
    """Serve the interactive cluster dashboard."""
    if cached_clusterer is None or cached_metadata is None:
        return "System not initialized", 500
    
    # Group birds by cluster
    clusters = {}
    for i, meta in enumerate(cached_metadata):
        cluster_id = int(cached_clusterer.cluster_labels_[i])
        if cluster_id not in clusters:
            clusters[cluster_id] = []
        
        clusters[cluster_id].append({
            'object_id': meta['object_id'],
            'confidence': float(meta['confidence']),
            'frame_id': meta['frame_id'],
            'index': i
        })
    
    # Calculate cluster statistics
    cluster_stats = {}
    for cluster_id, birds in clusters.items():
        cluster_stats[cluster_id] = {
            'count': len(birds),
            'avg_confidence': sum(bird['confidence'] for bird in birds) / len(birds),
            'confidence_range': [
                min(bird['confidence'] for bird in birds),
                max(bird['confidence'] for bird in birds)
            ]
        }
    
    html = create_cluster_dashboard_html(clusters, cluster_stats)
    return html

@app.route('/scatter')
def scatter():
    """Serve the scatter plot visualization."""
    if cached_visualizer is None:
        return "System not initialized."
    
    fig = cached_visualizer.create_scatter_plot(save_html=False)
    return fig.to_html(include_plotlyjs='cdn')


@app.route('/api/objects')
def api_objects():
    """Get all objects with cluster information."""
    if cached_clusterer is None:
        return jsonify({'error': 'System not initialized'})
    
    data = []
    for i, meta in enumerate(cached_metadata):
        obj_data = meta.copy()
        obj_data['cluster'] = int(cached_clusterer.cluster_labels_[i])
        
        if cached_clusterer.features_2d_ is not None:
            obj_data['x'] = float(cached_clusterer.features_2d_[i, 0])
            obj_data['y'] = float(cached_clusterer.features_2d_[i, 1])
        
        data.append(obj_data)
    
    return jsonify(data)

@app.route('/api/bird-image/<object_id>')
def api_bird_image(object_id):
    """Serve bird image by object ID."""
    import os
    from pathlib import Path
    
    # Get project root and construct image path  
    project_root = Path(__file__).parent.parent.parent
    image_path = project_root / "data" / "objects" / f"{object_id}.jpg"
    
    if image_path.exists():
        return send_file(str(image_path), mimetype='image/jpeg')
    else:
        # Return a placeholder SVG if image not found
        placeholder_svg = '''<svg xmlns="http://www.w3.org/2000/svg" width="60" height="60">
            <rect width="60" height="60" fill="#333"/>
            <text x="30" y="35" text-anchor="middle" fill="white" font-size="12">🐦</text>
        </svg>'''
        return placeholder_svg, 200, {'Content-Type': 'image/svg+xml'}

@app.route('/api/bird-details/<object_id>')
def api_bird_details(object_id):
    """Get detailed information about a specific bird."""
    if cached_clusterer is None or cached_metadata is None:
        return jsonify({'error': 'System not initialized'})
    
    # Find the bird in our metadata
    for i, meta in enumerate(cached_metadata):
        if meta['object_id'] == object_id:
            return jsonify({
                'object_id': meta['object_id'],
                'frame_id': meta['frame_id'],
                'confidence': float(meta['confidence']),
                'cluster': int(cached_clusterer.cluster_labels_[i])
            })
    
    return jsonify({'error': 'Bird not found'}), 404

def create_cluster_dashboard_html(clusters, cluster_stats):
    """Create interactive cluster dashboard HTML."""
    
    # Generate cluster cards HTML
    cluster_cards = []
    for cluster_id in sorted(clusters.keys()):
        birds = clusters[cluster_id]
        stats = cluster_stats[cluster_id]
        
        # Generate bird thumbnails for this cluster
        bird_thumbnails = []
        for bird in birds:
            bird_thumbnails.append(f"""
                <div class="bird-thumbnail" onclick="showBirdDetails('{bird['object_id']}')">
                    <img src="/api/bird-image/{bird['object_id']}" alt="Bird {bird['object_id']}" 
                         onerror="this.src='data:image/svg+xml,<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"60\" height=\"60\"><rect width=\"60\" height=\"60\" fill=\"%23333\"/><text x=\"30\" y=\"35\" text-anchor=\"middle\" fill=\"white\" font-size=\"12\">🐦</text></svg>'">
                    <div class="bird-info">
                        <div class="confidence">{bird['confidence']:.3f}</div>
                    </div>
                </div>
            """)
        
        cluster_cards.append(f"""
            <div class="cluster-card">
                <div class="cluster-header">
                    <h3>🐦 Cluster {cluster_id}</h3>
                    <div class="cluster-stats">
                        <span class="stat">Birds: {stats['count']}</span>
                        <span class="stat">Avg Confidence: {stats['avg_confidence']:.3f}</span>
                        <span class="stat">Range: {stats['confidence_range'][0]:.3f} - {stats['confidence_range'][1]:.3f}</span>
                    </div>
                </div>
                <div class="birds-grid">
                    {''.join(bird_thumbnails)}
                </div>
            </div>
        """)
    
    html = f"""
    <!DOCTYPE html>
    <html>
    <head>
        <title>Bird Cluster Dashboard</title>
        <style>
            body {{
                font-family: Arial, sans-serif;
                margin: 0;
                padding: 20px;
                background-color: #1a1a1a;
                color: #ffffff;
            }}
            
            .container {{ 
                max-width: 1200px; 
                margin: 0 auto; 
            }}
            
            h1 {{ 
                color: #FF6B35; 
                text-align: center; 
            }}
            
            .nav-links {{
                text-align: center;
                margin: 20px 0;
                padding: 10px;
                background: #333;
                border-radius: 8px;
            }}
            
            .nav-link {{
                color: #4CAF50;
                text-decoration: none;
                padding: 8px 16px;
                border-radius: 4px;
                font-weight: bold;
                margin: 0 5px;
            }}
            
            .nav-link:hover {{
                background: #4CAF50;
                color: #1a1a1a;
            }}
            
            .nav-current {{
                color: #FF6B35;
                font-weight: bold;
                padding: 8px 16px;
            }}
            
            .nav-separator {{
                color: #666;
                margin: 0 10px;
            }}
            
            .dashboard-header {{
                text-align: center;
                margin-bottom: 30px;
            }}
            
            .dashboard-summary {{
                background: #2a2a2a;
                padding: 20px;
                border-radius: 8px;
                margin-bottom: 30px;
                text-align: center;
            }}
            
            .summary-stats {{
                display: flex;
                justify-content: center;
                gap: 30px;
                flex-wrap: wrap;
            }}
            
            .summary-stat {{
                background: #333;
                padding: 15px 20px;
                border-radius: 8px;
                min-width: 120px;
            }}
            
            .stat-number {{
                font-size: 24px;
                font-weight: bold;
                color: #4CAF50;
            }}
            
            .stat-label {{
                font-size: 12px;
                color: #ccc;
                margin-top: 5px;
            }}
            
            .clusters-container {{
                display: grid;
                gap: 20px;
                grid-template-columns: repeat(auto-fit, minmax(400px, 1fr));
            }}
            
            .cluster-card {{
                background: #2a2a2a;
                border-radius: 12px;
                padding: 20px;
                border: 2px solid #333;
                transition: border-color 0.3s ease;
            }}
            
            .cluster-card:hover {{
                border-color: #4CAF50;
            }}
            
            .cluster-header {{
                display: flex;
                justify-content: space-between;
                align-items: center;
                margin-bottom: 15px;
                flex-wrap: wrap;
            }}
            
            .cluster-header h3 {{
                margin: 0;
                color: #4CAF50;
                font-size: 18px;
            }}
            
            .cluster-stats {{
                display: flex;
                gap: 15px;
                flex-wrap: wrap;
            }}
            
            .stat {{
                background: #333;
                padding: 5px 10px;
                border-radius: 4px;
                font-size: 12px;
                color: #ccc;
            }}
            
            .birds-grid {{
                display: grid;
                grid-template-columns: repeat(auto-fill, minmax(80px, 1fr));
                gap: 10px;
            }}
            
            .bird-thumbnail {{
                background: #333;
                border-radius: 8px;
                padding: 8px;
                text-align: center;
                cursor: pointer;
                transition: all 0.3s ease;
                position: relative;
            }}
            
            .bird-thumbnail:hover {{
                background: #4CAF50;
                transform: translateY(-2px);
            }}
            
            .bird-thumbnail img {{
                width: 60px;
                height: 60px;
                object-fit: cover;
                border-radius: 4px;
                background: #444;
            }}
            
            .bird-info {{
                margin-top: 5px;
            }}
            
            .confidence {{
                font-size: 10px;
                color: #ccc;
                font-weight: bold;
            }}
            
            .modal {{
                display: none;
                position: fixed;
                z-index: 1000;
                left: 0;
                top: 0;
                width: 100%;
                height: 100%;
                background-color: rgba(0,0,0,0.8);
            }}
            
            .modal-content {{
                background-color: #2a2a2a;
                margin: 5% auto;
                padding: 20px;
                border-radius: 12px;
                width: 80%;
                max-width: 500px;
                color: white;
            }}
            
            .close {{
                color: #aaa;
                float: right;
                font-size: 28px;
                font-weight: bold;
                cursor: pointer;
            }}
            
            .close:hover {{
                color: #fff;
            }}
        </style>
    </head>
    <body>
        <div class="container">
            <h1>🔬 Birds of Play - Bird Clustering</h1>
            <p style="text-align: center; color: #ccc;">Unsupervised ML analysis of detected bird objects</p>
            
            <nav class="nav-links">
                <a href="http://localhost:3000" class="nav-link" target="_self">
                    📹 Motion Detection Frames
                </a>
                <span class="nav-separator">|</span>
                <a href="http://localhost:3001" class="nav-link" target="_self">
                    🎯 Object Detections
                </a>
                <span class="nav-separator">|</span>
                <span class="nav-current">🔬 Bird Clustering</span>
                <span class="nav-separator">|</span>
                <a href="http://localhost:3003" class="nav-link" target="_self">
                    🧠 Fine-Tuning
                </a>
            </nav>
        </div>
        
        <div class="dashboard-summary">
            <div class="summary-stats">
                <div class="summary-stat">
                    <div class="stat-number">{sum(len(birds) for birds in clusters.values())}</div>
                    <div class="stat-label">Total Birds</div>
                </div>
                <div class="summary-stat">
                    <div class="stat-number">{len(clusters)}</div>
                    <div class="stat-label">Clusters Found</div>
                </div>
                <div class="summary-stat">
                    <div class="stat-number">{max(len(birds) for birds in clusters.values())}</div>
                    <div class="stat-label">Largest Cluster</div>
                </div>
                <div class="summary-stat">
                    <div class="stat-number">{sum(stats['avg_confidence'] for stats in cluster_stats.values()) / len(cluster_stats):.3f}</div>
                    <div class="stat-label">Avg Confidence</div>
                </div>
            </div>
        </div>
        
        <div class="clusters-container">
            {''.join(cluster_cards)}
        </div>
        
        <!-- Additional Tools -->
        <div style="text-align: center; margin-top: 30px; padding: 20px; background: #333; border-radius: 8px;">
            <h3>🔗 Additional Tools</h3>
            <div style="display: flex; justify-content: center; gap: 20px; flex-wrap: wrap; margin-top: 15px;">
                <a href="/scatter" class="nav-link" style="display: inline-block; padding: 12px 20px;">📊 Scatter Plot</a>
                <a href="/api/objects" class="nav-link" style="display: inline-block; padding: 12px 20px;">🐦 API Data</a>
            </div>
            <div style="color: #ccc; font-size: 12px; margin-top: 10px;">
                View 2D visualization or access complete bird data with clustering results
            </div>
        </div>
        
        <!-- Bird Details Modal -->
        <div id="birdModal" class="modal">
            <div class="modal-content">
                <span class="close">&times;</span>
                <div id="birdDetails">
                    <p>Loading bird details...</p>
                </div>
            </div>
        </div>
        
        <script>
            function showBirdDetails(objectId) {{
                const modal = document.getElementById('birdModal');
                const details = document.getElementById('birdDetails');
                
                // Show modal
                modal.style.display = 'block';
                
                // Load bird details
                fetch(`/api/bird-details/${{objectId}}`)
                    .then(response => response.json())
                    .then(data => {{
                        details.innerHTML = `
                            <h3>🐦 Bird Details</h3>
                            <p><strong>Object ID:</strong> ${{data.object_id}}</p>
                            <p><strong>Frame ID:</strong> ${{data.frame_id}}</p>
                            <p><strong>Cluster:</strong> ${{data.cluster}}</p>
                            <p><strong>Confidence:</strong> ${{data.confidence.toFixed(3)}}</p>
                            <img src="/api/bird-image/${{objectId}}" style="max-width: 100%; border-radius: 8px; margin-top: 10px;">
                        `;
                    }})
                    .catch(error => {{
                        details.innerHTML = `<p>Error loading bird details: ${{error}}</p>`;
                    }});
            }}
            
            // Close modal when clicking X or outside
            document.querySelector('.close').onclick = function() {{
                document.getElementById('birdModal').style.display = 'none';
            }}
            
            window.onclick = function(event) {{
                const modal = document.getElementById('birdModal');
                if (event.target == modal) {{
                    modal.style.display = 'none';
                }}
            }}
        </script>
    </body>
    </html>
    """
    
    return html

if __name__ == '__main__':
    # Use configuration for server settings
    config = cached_config if cached_config else load_clustering_config()
    
    # Auto-initialize if configured to do so
    if config and config.auto_initialize:
        print("Auto-initializing system...")
        initialize_system()
    
    port = config.port if config else 3002
    host = config.host if config else '0.0.0.0'
    debug_mode = config.debug_mode if config else False
    
    print(f"🔬 Starting Bird Clustering Server on http://localhost:{port}")
    print("📊 Navigate to http://localhost:3002 to explore clustering results")
    print(f"⚙️  Configuration: auto_initialize={config.auto_initialize if config else False}, debug={debug_mode}")
    
    try:
        app.run(host=host, port=port, debug=debug_mode)
    except Exception as e:
        print(f"Error starting server: {e}")
        print(f"Make sure port {port} is available")
