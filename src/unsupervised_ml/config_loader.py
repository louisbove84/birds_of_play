"""
Configuration loader for Birds of Play Unsupervised ML system.
"""

import yaml
import logging
from pathlib import Path
from typing import Dict, Any, Optional
from dataclasses import dataclass

@dataclass
class ClusteringConfig:
    """Configuration data class for easy access to clustering parameters."""
    
    # Feature extraction
    model_name: str
    use_gpu: bool
    min_confidence: float
    batch_size: int
    image_size: int
    
    # Ward linkage thresholds
    ward_conservative: float
    ward_balanced: float
    ward_permissive: float
    
    # Average linkage thresholds
    average_conservative: float
    average_balanced: float
    average_permissive: float
    
    # Evaluation settings
    min_species: int
    max_species: int
    sweet_spot_min: int
    sweet_spot_max: int
    
    # Scoring weights
    silhouette_weight: float
    species_count_bonus: float
    ward_method_bonus: float
    
    # Penalties
    too_few_penalty: float
    too_many_penalty: float
    
    # Visualization
    reduction_method: str
    n_components: int
    
    # Web interface
    host: str
    port: int
    debug_mode: bool
    auto_initialize: bool
    
    # Data management
    mongodb_uri: str
    database_name: str
    objects_directory: str
    
    # Performance
    max_objects_per_batch: int
    cache_features: bool
    n_jobs: int
    
    # Logging
    log_level: str
    log_clustering_steps: bool
    
    # Testing
    test_video_path: str
    clear_database_before_test: bool
    save_test_results: bool
    test_timeout_seconds: int


class ConfigLoader:
    """Loads and validates clustering configuration from YAML file."""
    
    def __init__(self, config_path: Optional[str] = None):
        if config_path is None:
            # Default to config file in same directory
            config_path = Path(__file__).parent / "clustering_config.yaml"
        
        self.config_path = Path(config_path)
        self.logger = logging.getLogger(__name__)
        
    def load_config(self) -> ClusteringConfig:
        """Load configuration from YAML file."""
        if not self.config_path.exists():
            raise FileNotFoundError(f"Configuration file not found: {self.config_path}")
        
        try:
            with open(self.config_path, 'r') as f:
                config_data = yaml.safe_load(f)
            
            self.logger.info(f"Loaded configuration from {self.config_path}")
            
            # Extract values with defaults
            feature_config = config_data.get('feature_extraction', {})
            clustering_config = config_data.get('clustering', {})
            ward_config = clustering_config.get('ward_linkage', {})
            average_config = clustering_config.get('average_linkage', {})
            eval_config = config_data.get('evaluation', {})
            ideal_range = eval_config.get('ideal_species_range', {})
            scoring_weights = eval_config.get('scoring_weights', {})
            penalties = eval_config.get('penalties', {})
            viz_config = config_data.get('visualization', {})
            web_config = config_data.get('web_interface', {})
            data_config = config_data.get('data', {})
            perf_config = config_data.get('performance', {})
            log_config = config_data.get('logging', {})
            test_config = config_data.get('testing', {})
            
            # Create configuration object
            config = ClusteringConfig(
                # Feature extraction
                model_name=feature_config.get('model_name', 'resnet50'),
                use_gpu=feature_config.get('use_gpu', True),
                min_confidence=feature_config.get('min_confidence', 0.5),
                batch_size=feature_config.get('batch_size', 32),
                image_size=feature_config.get('image_size', 224),
                
                # Ward linkage
                ward_conservative=ward_config.get('conservative_threshold', 50.0),
                ward_balanced=ward_config.get('balanced_threshold', 75.0),
                ward_permissive=ward_config.get('permissive_threshold', 90.0),
                
                # Average linkage
                average_conservative=average_config.get('conservative_threshold', 45.0),
                average_balanced=average_config.get('balanced_threshold', 70.0),
                average_permissive=average_config.get('permissive_threshold', 85.0),
                
                # Evaluation
                min_species=ideal_range.get('min_species', 2),
                max_species=ideal_range.get('max_species', 6),
                sweet_spot_min=ideal_range.get('sweet_spot_min', 2),
                sweet_spot_max=ideal_range.get('sweet_spot_max', 6),
                
                # Scoring
                silhouette_weight=scoring_weights.get('silhouette_weight', 1.0),
                species_count_bonus=scoring_weights.get('species_count_bonus', 0.1),
                ward_method_bonus=scoring_weights.get('ward_method_bonus', 0.05),
                
                # Penalties
                too_few_penalty=penalties.get('too_few_species', -1.0),
                too_many_penalty=penalties.get('too_many_species', -0.5),
                
                # Visualization
                reduction_method=viz_config.get('reduction_method', 'tsne'),
                n_components=viz_config.get('n_components', 2),
                
                # Web interface
                host=web_config.get('host', '0.0.0.0'),
                port=web_config.get('port', 3002),
                debug_mode=web_config.get('debug_mode', False),
                auto_initialize=web_config.get('auto_initialize', False),
                
                # Data
                mongodb_uri=data_config.get('mongodb_uri', 'mongodb://localhost:27017/'),
                database_name=data_config.get('database_name', 'birds_of_play'),
                objects_directory=data_config.get('objects_directory', 'data/objects'),
                
                # Performance
                max_objects_per_batch=perf_config.get('max_objects_per_batch', 100),
                cache_features=perf_config.get('cache_features', True),
                n_jobs=perf_config.get('n_jobs', -1),
                
                # Logging
                log_level=log_config.get('level', 'INFO'),
                log_clustering_steps=log_config.get('log_clustering_steps', True),
                
                # Testing
                test_video_path=test_config.get('test_video_path', 'test/vid/vid_1.mp4'),
                clear_database_before_test=test_config.get('clear_database_before_test', True),
                save_test_results=test_config.get('save_test_results', True),
                test_timeout_seconds=test_config.get('test_timeout_seconds', 300)
            )
            
            self._validate_config(config)
            return config
            
        except yaml.YAMLError as e:
            raise ValueError(f"Invalid YAML in configuration file: {e}")
        except Exception as e:
            raise ValueError(f"Error loading configuration: {e}")
    
    def _validate_config(self, config: ClusteringConfig) -> None:
        """Validate configuration values."""
        # Validate thresholds
        if config.ward_conservative <= 0 or config.ward_balanced <= 0 or config.ward_permissive <= 0:
            raise ValueError("Ward linkage thresholds must be positive")
            
        if config.average_conservative <= 0 or config.average_balanced <= 0 or config.average_permissive <= 0:
            raise ValueError("Average linkage thresholds must be positive")
        
        # Validate threshold ordering
        if not (config.ward_conservative <= config.ward_balanced <= config.ward_permissive):
            self.logger.warning("Ward thresholds not in ascending order (conservative ≤ balanced ≤ permissive)")
            
        if not (config.average_conservative <= config.average_balanced <= config.average_permissive):
            self.logger.warning("Average thresholds not in ascending order (conservative ≤ balanced ≤ permissive)")
        
        # Validate species ranges
        if config.min_species < 1:
            raise ValueError("min_species must be at least 1")
        if config.max_species <= config.min_species:
            raise ValueError("max_species must be greater than min_species")
            
        # Validate confidence
        if not (0.0 <= config.min_confidence <= 1.0):
            raise ValueError("min_confidence must be between 0.0 and 1.0")
            
        # Validate batch size
        if config.batch_size < 1:
            raise ValueError("batch_size must be at least 1")
            
        # Validate port
        if not (1024 <= config.port <= 65535):
            raise ValueError("port must be between 1024 and 65535")
        
        self.logger.info("Configuration validation passed")


def load_clustering_config(config_path: Optional[str] = None) -> ClusteringConfig:
    """
    Convenience function to load clustering configuration.
    
    Args:
        config_path: Path to config file (defaults to clustering_config.yaml in same directory)
        
    Returns:
        ClusteringConfig object with all parameters
    """
    loader = ConfigLoader(config_path)
    return loader.load_config()


# Example usage:
if __name__ == "__main__":
    # Test configuration loading
    try:
        config = load_clustering_config()
        print("✅ Configuration loaded successfully!")
        print(f"📊 Ward balanced threshold: {config.ward_balanced}")
        print(f"🎯 Target species range: {config.min_species}-{config.max_species}")
        print(f"🔬 Model: {config.model_name}")
        print(f"🌐 Server: {config.host}:{config.port}")
    except Exception as e:
        print(f"❌ Configuration error: {e}")
