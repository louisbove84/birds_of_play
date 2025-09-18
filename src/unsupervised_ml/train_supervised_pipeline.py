"""
Complete pipeline: Clustering → Supervised Training → Model Evaluation
"""

import sys
from pathlib import Path
import logging
from typing import Dict, List

# Add project root to path
project_root = Path(__file__).resolve().parent.parent.parent
sys.path.insert(0, str(project_root))

from object_data_manager import ObjectDataManager
from feature_extractor import FeatureExtractor, FeaturePipeline
from bird_clusterer import BirdClusterer, ClusteringExperiment
from supervised_classifier import SupervisedBirdTrainer
from config_loader import load_clustering_config

def run_complete_pipeline(save_model: bool = True, model_path: str = "trained_bird_classifier.pth"):
    """
    Run the complete pipeline from clustering to supervised training.
    
    Args:
        save_model: Whether to save the trained model
        model_path: Path to save the model
        
    Returns:
        Dictionary with pipeline results
    """
    
    # Load configuration
    config = load_clustering_config()
    
    # Set up logging
    log_level = getattr(logging, config.log_level.upper(), logging.INFO)
    logging.basicConfig(level=log_level, format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')
    logger = logging.getLogger(__name__)
    
    logger.info("🚀 Starting Complete Supervised Learning Pipeline")
    
    results = {}
    
    try:
        # Step 1: Load bird data and extract features
        logger.info("📊 Step 1: Loading bird data and extracting features")
        
        with ObjectDataManager() as data_manager:
            feature_extractor = FeatureExtractor(model_name=config.model_name)
            pipeline = FeaturePipeline(data_manager, feature_extractor)
            
            features, metadata = pipeline.extract_all_features(min_confidence=config.min_confidence)
            
            if len(features) == 0:
                logger.error("No bird objects found for training")
                return {'error': 'No bird objects found'}
            
            logger.info(f"✅ Loaded {len(features)} bird objects")
            results['n_objects'] = len(features)
        
        # Step 2: Perform clustering to generate pseudo-labels
        logger.info("🔬 Step 2: Performing clustering for pseudo-labels")
        
        experiment = ClusteringExperiment(features, metadata, config=config)
        clustering_results = experiment.run_all_methods()
        best_method, best_result = experiment.get_best_method()
        
        if not best_result:
            logger.error("Clustering failed")
            return {'error': 'Clustering failed'}
        
        best_clusterer = best_result['clusterer']
        n_clusters = best_result['metrics']['n_clusters']
        silhouette_score = best_result['metrics']['silhouette_score']
        
        logger.info(f"✅ Best clustering: {best_method}")
        logger.info(f"   - {n_clusters} bird species clusters")
        logger.info(f"   - Silhouette score: {silhouette_score:.3f}")
        
        results['clustering'] = {
            'best_method': best_method,
            'n_clusters': n_clusters,
            'silhouette_score': silhouette_score,
            'all_methods': {name: res.get('metrics', {}) for name, res in clustering_results.items()}
        }
        
        # Step 3: Train supervised classifier with pseudo-labels
        logger.info("🧠 Step 3: Training supervised classifier")
        
        trainer = SupervisedBirdTrainer(config=config)
        
        # Prepare data from clustering results
        success = trainer.prepare_data_from_clustering(best_clusterer, metadata)
        if not success:
            logger.error("Failed to prepare training data")
            return {'error': 'Failed to prepare training data'}
        
        logger.info(f"✅ Prepared training data with {trainer.training_metadata['n_classes']} classes")
        
        # Phase 1: Train with frozen backbone
        logger.info("🔒 Phase 1: Training with frozen backbone")
        trainer.train_phase1_frozen_backbone(epochs=10, lr=0.001)
        
        # Phase 2: Fine-tune entire model
        logger.info("🔓 Phase 2: Fine-tuning entire model")
        trainer.train_phase2_full_finetuning(epochs=5, lr=0.0001)
        
        # Step 4: Evaluate model
        logger.info("📈 Step 4: Evaluating trained model")
        
        evaluation = trainer.evaluate()
        
        logger.info(f"✅ Final Test Accuracy: {evaluation['accuracy']:.4f}")
        logger.info(f"   - Mean Confidence: {evaluation['mean_confidence']:.3f}")
        logger.info(f"   - Low Confidence Threshold: {evaluation['low_confidence_threshold']:.3f}")
        
        results['training'] = {
            'training_metadata': trainer.training_metadata,
            'train_history': trainer.train_history,
            'evaluation': evaluation
        }
        
        # Step 5: Save model if requested
        if save_model:
            logger.info(f"💾 Step 5: Saving model to {model_path}")
            trainer.save_model(model_path)
            results['model_path'] = model_path
        
        # Step 6: Generate sample predictions for fine-tuning server
        logger.info("🎯 Step 6: Generating sample predictions for active learning")
        
        # Get some sample images for prediction
        project_root = Path(__file__).parent.parent.parent
        objects_dir = project_root / "data" / "objects"
        sample_images = list(objects_dir.glob("*.jpg"))[:10]  # First 10 images
        
        if sample_images:
            sample_predictions = trainer.predict_with_confidence(
                [str(img) for img in sample_images], top_k=3
            )
            
            # Find low confidence predictions for active learning
            low_conf_threshold = evaluation['low_confidence_threshold']
            low_confidence_samples = [
                pred for pred in sample_predictions 
                if pred.get('max_confidence', 1.0) < low_conf_threshold
            ]
            
            logger.info(f"✅ Found {len(low_confidence_samples)} low-confidence samples for active learning")
            
            results['active_learning'] = {
                'total_samples': len(sample_predictions),
                'low_confidence_samples': len(low_confidence_samples),
                'low_confidence_threshold': low_conf_threshold,
                'sample_predictions': sample_predictions[:5]  # First 5 for demo
            }
        
        logger.info("🎉 Pipeline completed successfully!")
        
        return results
        
    except Exception as e:
        logger.error(f"❌ Pipeline failed: {e}")
        import traceback
        traceback.print_exc()
        return {'error': str(e)}


def print_pipeline_summary(results: Dict):
    """Print a summary of pipeline results."""
    
    if 'error' in results:
        print(f"❌ Pipeline failed: {results['error']}")
        return
    
    print("\n" + "="*60)
    print("🎉 SUPERVISED LEARNING PIPELINE SUMMARY")
    print("="*60)
    
    # Data summary
    print(f"\n📊 DATA:")
    print(f"   • Total bird objects: {results['n_objects']}")
    
    # Clustering summary
    clustering = results['clustering']
    print(f"\n🔬 CLUSTERING:")
    print(f"   • Best method: {clustering['best_method']}")
    print(f"   • Species clusters: {clustering['n_clusters']}")
    print(f"   • Silhouette score: {clustering['silhouette_score']:.3f}")
    
    # Training summary
    training = results['training']
    metadata = training['training_metadata']
    evaluation = training['evaluation']
    
    print(f"\n🧠 TRAINING:")
    print(f"   • Classes: {metadata['n_classes']}")
    print(f"   • Train samples: {metadata['train_size']}")
    print(f"   • Validation samples: {metadata['val_size']}")
    print(f"   • Test samples: {metadata['test_size']}")
    
    print(f"\n📈 EVALUATION:")
    print(f"   • Test accuracy: {evaluation['accuracy']:.4f}")
    print(f"   • Mean confidence: {evaluation['mean_confidence']:.3f}")
    print(f"   • Low confidence threshold: {evaluation['low_confidence_threshold']:.3f}")
    
    # Active learning summary
    if 'active_learning' in results:
        al = results['active_learning']
        print(f"\n🎯 ACTIVE LEARNING:")
        print(f"   • Total predictions: {al['total_samples']}")
        print(f"   • Low confidence samples: {al['low_confidence_samples']}")
        print(f"   • Ready for fine-tuning server!")
    
    if 'model_path' in results:
        print(f"\n💾 MODEL SAVED: {results['model_path']}")
    
    print("\n" + "="*60)
    print("🚀 Next steps:")
    print("   1. Start fine-tuning server: python fine_tuning_server.py")
    print("   2. Navigate to http://localhost:3003")
    print("   3. Label low-confidence predictions")
    print("   4. Retrain model with corrected labels")
    print("="*60)


if __name__ == "__main__":
    # Run the complete pipeline
    results = run_complete_pipeline(save_model=True, model_path="trained_bird_classifier.pth")
    
    # Print summary
    print_pipeline_summary(results)
