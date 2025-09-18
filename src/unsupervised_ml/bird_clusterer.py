"""
Bird Clustering module for grouping similar bird objects.
"""

import numpy as np
import pandas as pd
from sklearn.cluster import AgglomerativeClustering
from sklearn.preprocessing import StandardScaler
from sklearn.manifold import TSNE
from sklearn.metrics import silhouette_score
from scipy.cluster.hierarchy import dendrogram, linkage, fcluster
from scipy.spatial.distance import pdist
try:
    from umap import UMAP
    HAS_UMAP = True
except ImportError:
    HAS_UMAP = False
from typing import Dict, List, Tuple, Optional
import logging
from config_loader import load_clustering_config, ClusteringConfig

class BirdClusterer:
    """Clustering algorithms for grouping similar bird images."""
    
    def __init__(self, config: Optional[ClusteringConfig] = None):
        # Load configuration
        self.config = config if config is not None else load_clustering_config()
        
        self.random_state = 42  # Will be configurable in YAML
        self.scaler = StandardScaler()
        self.clusterer = None
        self.cluster_labels_ = None
        self.features_scaled_ = None
        self.features_2d_ = None
        self.metadata_ = None
        
        # Set up logging
        log_level = getattr(logging, self.config.log_level.upper(), logging.INFO)
        logging.basicConfig(level=log_level)
        self.logger = logging.getLogger(__name__)
    
    def preprocess_features(self, features: np.ndarray) -> np.ndarray:
        """Preprocess features by scaling."""
        self.logger.info(f"Preprocessing features of shape {features.shape}")
        return self.scaler.fit_transform(features)
    
    def reduce_dimensions(self, features: np.ndarray, method: str = 'tsne', 
                         n_components: int = 2) -> np.ndarray:
        """Reduce dimensionality for visualization."""
        self.logger.info(f"Reducing dimensions using {method} to {n_components}D")
        
        if method == 'tsne':
            reducer = TSNE(n_components=n_components, random_state=self.random_state, 
                          perplexity=min(30, len(features)-1))
        elif method == 'umap' and HAS_UMAP:
            reducer = UMAP(n_components=n_components, random_state=self.random_state,
                          n_neighbors=min(15, len(features)-1))
        else:
            # Fallback to t-SNE
            reducer = TSNE(n_components=n_components, random_state=self.random_state, 
                          perplexity=min(30, len(features)-1))
        
        return reducer.fit_transform(features)
    
    def cluster_ward_distance(self, features: np.ndarray, distance_threshold: Optional[float] = None) -> np.ndarray:
        """
        Perform Ward linkage hierarchical clustering with distance threshold.
        Primary method for compact, variance-minimizing clusters.
        Optimized for SimCLR features (512D ResNet-18 vectors).
        """
        # Use config value if not provided
        if distance_threshold is None:
            distance_threshold = self.config.ward_balanced
            
        self.logger.info(f"Performing Ward linkage clustering with distance_threshold={distance_threshold}")
        
        clusterer = AgglomerativeClustering(
            n_clusters=None, 
            distance_threshold=distance_threshold,
            linkage='ward',
            metric='euclidean'  # Ward requires Euclidean distance
        )
        labels = clusterer.fit_predict(features)
        
        n_clusters = len(set(labels))
        self.logger.info(f"Ward linkage found {n_clusters} bird species clusters")
        
        self.clusterer = clusterer
        return labels
    
    def cluster_average_distance(self, features: np.ndarray, distance_threshold: Optional[float] = None) -> np.ndarray:
        """
        Perform Average linkage hierarchical clustering with distance threshold.
        Alternate method for handling varied cluster shapes.
        Compatible with SimCLR features using Euclidean distance.
        """
        # Use config value if not provided
        if distance_threshold is None:
            distance_threshold = self.config.average_balanced
            
        self.logger.info(f"Performing Average linkage clustering with distance_threshold={distance_threshold}")
        
        clusterer = AgglomerativeClustering(
            n_clusters=None,
            distance_threshold=distance_threshold, 
            linkage='average',
            metric='euclidean'
        )
        labels = clusterer.fit_predict(features)
        
        n_clusters = len(set(labels))
        self.logger.info(f"Average linkage found {n_clusters} bird species clusters")
        
        self.clusterer = clusterer
        return labels
    
    def evaluate_clustering(self, features: np.ndarray, labels: np.ndarray) -> Dict[str, float]:
        """Evaluate clustering quality."""
        n_clusters = len(set(labels))
        metrics = {
            'n_clusters': n_clusters,
            'n_noise': 0,  # Hierarchical clustering doesn't have noise points
            'n_points': len(labels)
        }
        
        # For hierarchical clustering, all points are assigned to clusters (no noise points)
        if n_clusters < 2:
            metrics['silhouette_score'] = -1.0  # Invalid clustering
            metrics['error'] = 'Less than 2 clusters found'
        else:
            try:
                metrics['silhouette_score'] = silhouette_score(features, labels)
            except Exception as e:
                self.logger.warning(f"Silhouette score calculation failed: {e}")
                metrics['silhouette_score'] = -1.0
        
        return metrics
    
    def fit_predict(self, features: np.ndarray, metadata: List[Dict], 
                   method: str = 'ward_distance', **kwargs) -> Tuple[np.ndarray, Dict]:
        """
        Fit clustering model and predict cluster labels.
        
        Args:
            features: SimCLR features (typically 512D from ResNet-18)
            metadata: Bird object metadata
            method: 'ward_distance' (primary) or 'average_distance' (alternate)
            **kwargs: distance_threshold (default 75.0 for ResNet50 2048D features)
        """
        self.features_scaled_ = self.preprocess_features(features)
        self.metadata_ = metadata
        
        distance_threshold = kwargs.get('distance_threshold', self.config.ward_balanced)
        
        if method == 'ward_distance':
            labels = self.cluster_ward_distance(self.features_scaled_, distance_threshold=distance_threshold)
        elif method == 'average_distance':
            labels = self.cluster_average_distance(self.features_scaled_, distance_threshold=distance_threshold)
        else:
            raise ValueError(f"Unsupported clustering method: {method}. Use 'ward_distance' or 'average_distance'")
        
        self.cluster_labels_ = labels
        
        # Generate 2D representation for visualization
        self.features_2d_ = self.reduce_dimensions(self.features_scaled_, method='tsne')
        
        # Evaluate clustering
        metrics = self.evaluate_clustering(self.features_scaled_, labels)
        
        return labels, metrics
    
    def get_cluster_summary(self) -> pd.DataFrame:
        """Get summary statistics for each cluster."""
        if self.cluster_labels_ is None or self.metadata_ is None:
            raise ValueError("Must fit model first")
        
        df = pd.DataFrame(self.metadata_)
        df['cluster'] = self.cluster_labels_
        
        summary = df.groupby('cluster').agg({
            'object_id': 'count',
            'confidence': ['mean', 'std'],
            'frame_id': 'nunique',
        }).round(3)
        
        summary.columns = ['_'.join(col).strip() for col in summary.columns.values]
        summary = summary.rename(columns={'object_id_count': 'n_objects'})
        
        return summary
    
    def get_dendrogram_data(self, linkage_method: str = 'ward') -> Dict:
        """
        Generate dendrogram data for hierarchical clustering visualization and threshold tuning.
        
        Args:
            linkage_method: 'ward' (primary) or 'average' (alternate)
            
        Returns:
            Dictionary with linkage matrix, labels, and suggested thresholds
        """
        if self.features_scaled_ is None:
            raise ValueError("Must fit model first")
        
        # Compute linkage matrix using Euclidean distance
        linkage_matrix = linkage(self.features_scaled_, method=linkage_method, metric='euclidean')
        
        # Analyze distances to suggest good thresholds
        distances = linkage_matrix[:, 2]  # Third column contains distances
        distance_gaps = np.diff(np.sort(distances))
        
        # Find largest gaps in distance (good threshold candidates)
        gap_indices = np.argsort(distance_gaps)[-5:]  # Top 5 gaps
        suggested_thresholds = [float(np.sort(distances)[i]) for i in gap_indices]
        
        return {
            'linkage_matrix': linkage_matrix.tolist(),
            'labels': [meta.get('object_id', f'Bird_{i}') for i, meta in enumerate(self.metadata_)],
            'method': linkage_method,
            'suggested_thresholds': sorted(suggested_thresholds),
            'distance_stats': {
                'min_distance': float(distances.min()),
                'max_distance': float(distances.max()),
                'mean_distance': float(distances.mean()),
                'std_distance': float(distances.std())
            }
        }
    
    def tune_distance_threshold(self, method: str = 'ward_distance', 
                              threshold_range: Tuple[float, float] = (0.5, 3.0),
                              n_thresholds: int = 10) -> Dict[float, Dict]:
        """
        Tune distance threshold by testing multiple values and evaluating results.
        
        Args:
            method: 'ward_distance' or 'average_distance'
            threshold_range: (min_threshold, max_threshold) to test
            n_thresholds: Number of threshold values to test
            
        Returns:
            Dictionary mapping thresholds to clustering results and metrics
        """
        if self.features_scaled_ is None:
            raise ValueError("Must fit model first")
            
        results = {}
        thresholds = np.linspace(threshold_range[0], threshold_range[1], n_thresholds)
        
        self.logger.info(f"Tuning distance threshold for {method} over {n_thresholds} values")
        
        for threshold in thresholds:
            try:
                if method == 'ward_distance':
                    labels = self.cluster_ward_distance(self.features_scaled_, distance_threshold=threshold)
                elif method == 'average_distance':
                    labels = self.cluster_average_distance(self.features_scaled_, distance_threshold=threshold)
                else:
                    continue
                    
                metrics = self.evaluate_clustering(self.features_scaled_, labels)
                
                results[float(threshold)] = {
                    'labels': labels,
                    'metrics': metrics,
                    'n_clusters': metrics['n_clusters']
                }
                
            except Exception as e:
                self.logger.warning(f"Failed threshold {threshold}: {e}")
                
        return results


class ClusteringExperiment:
    """Run clustering experiments with multiple algorithms."""
    
    def __init__(self, features: np.ndarray, metadata: List[Dict], config: Optional[ClusteringConfig] = None):
        self.features = features
        self.metadata = metadata
        self.results = {}
        self.config = config if config is not None else load_clustering_config()
        
        # Set up logging
        log_level = getattr(logging, self.config.log_level.upper(), logging.INFO)
        logging.basicConfig(level=log_level)
        self.logger = logging.getLogger(__name__)
    
    def run_all_methods(self) -> Dict[str, Dict]:
        """
        Run clustering experiments with Ward and Average linkage at different distance thresholds.
        Automatically determines optimal number of bird species clusters.
        """
        self.logger.info("Running bird species clustering experiment with distance-based methods")
        
        methods = {
            # Ward linkage (primary method) - compact, variance-minimizing clusters
            # Thresholds loaded from configuration file
            'ward_conservative': {'method': 'ward_distance', 'distance_threshold': self.config.ward_conservative},
            'ward_balanced': {'method': 'ward_distance', 'distance_threshold': self.config.ward_balanced},
            'ward_permissive': {'method': 'ward_distance', 'distance_threshold': self.config.ward_permissive},
            
            # Average linkage (alternate method) - handles varied cluster shapes  
            'average_conservative': {'method': 'average_distance', 'distance_threshold': self.config.average_conservative},
            'average_balanced': {'method': 'average_distance', 'distance_threshold': self.config.average_balanced},
            'average_permissive': {'method': 'average_distance', 'distance_threshold': self.config.average_permissive},
        }
        
        for name, params in methods.items():
            try:
                clusterer = BirdClusterer(config=self.config)
                method = params.pop('method')
                labels, metrics = clusterer.fit_predict(self.features, self.metadata, 
                                                       method=method, **params)
                
                self.results[name] = {
                    'labels': labels,
                    'metrics': metrics,
                    'clusterer': clusterer,
                    'method': method,
                    'params': params
                }
                
                self.logger.info(f"Completed {name}: {metrics.get('n_clusters', 0)} clusters")
                
            except Exception as e:
                self.logger.error(f"Failed {name}: {e}")
                self.results[name] = {'error': str(e)}
        
        return self.results
    
    def get_best_method(self) -> Tuple[str, Dict]:
        """
        Get the best performing clustering method for bird species identification.
        Prioritizes methods with reasonable number of species (2-10) and good silhouette score.
        """
        valid_results = {name: res for name, res in self.results.items() 
                        if 'error' not in res and 'metrics' in res}
        
        if not valid_results:
            return None, {}
        
        def scoring_function(name):
            metrics = valid_results[name]['metrics']
            silhouette = metrics.get('silhouette_score', -2)
            n_clusters = metrics.get('n_clusters', 0)
            
            # Penalty for unrealistic number of species (from config)
            if n_clusters < self.config.min_species:
                cluster_penalty = self.config.too_few_penalty
            elif n_clusters > self.config.max_species:
                cluster_penalty = self.config.too_many_penalty
            elif self.config.sweet_spot_min <= n_clusters <= self.config.sweet_spot_max:
                cluster_penalty = self.config.species_count_bonus
            else:
                cluster_penalty = 0.0   # Acceptable range
                
            # Prefer Ward linkage (primary method) slightly
            method_bonus = self.config.ward_method_bonus if 'ward' in name else 0.0
            
            return (silhouette * self.config.silhouette_weight) + cluster_penalty + method_bonus
        
        best_name = max(valid_results.keys(), key=scoring_function)
        
        return best_name, valid_results[best_name]
    
    def summary_report(self) -> pd.DataFrame:
        """Generate summary report of all clustering methods."""
        rows = []
        
        for name, result in self.results.items():
            if 'error' in result:
                rows.append({
                    'method': name,
                    'status': 'failed',
                    'error': result['error']
                })
            else:
                metrics = result.get('metrics', {})
                rows.append({
                    'method': name,
                    'status': 'success',
                    'n_clusters': metrics.get('n_clusters', 0),
                    'silhouette_score': round(metrics.get('silhouette_score', -1), 3),
                })
        
        return pd.DataFrame(rows)
