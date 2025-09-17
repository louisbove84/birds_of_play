"""
Bird Clustering module for grouping similar bird objects.
"""

import numpy as np
import pandas as pd
from sklearn.cluster import KMeans, DBSCAN
from sklearn.preprocessing import StandardScaler
from sklearn.manifold import TSNE
from sklearn.metrics import silhouette_score
try:
    from umap import UMAP
    HAS_UMAP = True
except ImportError:
    HAS_UMAP = False
from typing import Dict, List, Tuple
import logging

class BirdClusterer:
    """Clustering algorithms for grouping similar bird images."""
    
    def __init__(self, random_state: int = 42):
        self.random_state = random_state
        self.scaler = StandardScaler()
        self.clusterer = None
        self.cluster_labels_ = None
        self.features_scaled_ = None
        self.features_2d_ = None
        self.metadata_ = None
        
        logging.basicConfig(level=logging.INFO)
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
    
    def cluster_kmeans(self, features: np.ndarray, n_clusters: int = 5) -> np.ndarray:
        """Perform K-means clustering."""
        self.logger.info(f"Performing K-means clustering with {n_clusters} clusters")
        
        clusterer = KMeans(n_clusters=n_clusters, random_state=self.random_state, n_init=10)
        labels = clusterer.fit_predict(features)
        
        self.clusterer = clusterer
        return labels
    
    def cluster_dbscan(self, features: np.ndarray, eps: float = 0.5, 
                      min_samples: int = 5) -> np.ndarray:
        """Perform DBSCAN clustering."""
        self.logger.info(f"Performing DBSCAN clustering with eps={eps}, min_samples={min_samples}")
        
        clusterer = DBSCAN(eps=eps, min_samples=min_samples)
        labels = clusterer.fit_predict(features)
        
        n_clusters = len(set(labels)) - (1 if -1 in labels else 0)
        n_noise = list(labels).count(-1)
        
        self.logger.info(f"Found {n_clusters} clusters and {n_noise} noise points")
        
        self.clusterer = clusterer
        return labels
    
    def evaluate_clustering(self, features: np.ndarray, labels: np.ndarray) -> Dict[str, float]:
        """Evaluate clustering quality."""
        mask = labels != -1
        if not mask.any():
            return {'error': 'All points classified as noise'}
        
        features_clean = features[mask]
        labels_clean = labels[mask]
        
        if len(set(labels_clean)) < 2:
            return {'error': 'Less than 2 clusters found'}
        
        metrics = {}
        
        try:
            metrics['silhouette_score'] = silhouette_score(features_clean, labels_clean)
        except:
            metrics['silhouette_score'] = -1
        
        metrics['n_clusters'] = len(set(labels_clean))
        metrics['n_noise'] = list(labels).count(-1)
        metrics['n_points'] = len(labels)
        
        return metrics
    
    def fit_predict(self, features: np.ndarray, metadata: List[Dict], 
                   method: str = 'kmeans', **kwargs) -> Tuple[np.ndarray, Dict]:
        """Fit clustering model and predict cluster labels."""
        self.features_scaled_ = self.preprocess_features(features)
        self.metadata_ = metadata
        
        if method == 'kmeans':
            n_clusters = kwargs.get('n_clusters', 5)
            labels = self.cluster_kmeans(self.features_scaled_, n_clusters=n_clusters)
        elif method == 'dbscan':
            eps = kwargs.get('eps', 0.5)
            min_samples = kwargs.get('min_samples', 5)
            labels = self.cluster_dbscan(self.features_scaled_, eps=eps, min_samples=min_samples)
        else:
            raise ValueError(f"Unsupported clustering method: {method}")
        
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


class ClusteringExperiment:
    """Run clustering experiments with multiple algorithms."""
    
    def __init__(self, features: np.ndarray, metadata: List[Dict]):
        self.features = features
        self.metadata = metadata
        self.results = {}
        
        logging.basicConfig(level=logging.INFO)
        self.logger = logging.getLogger(__name__)
    
    def run_all_methods(self) -> Dict[str, Dict]:
        """Run clustering with multiple methods and parameters."""
        self.logger.info("Running clustering experiment with all methods")
        
        methods = {
            'kmeans_3': {'method': 'kmeans', 'n_clusters': 3},
            'kmeans_5': {'method': 'kmeans', 'n_clusters': 5},
            'dbscan_loose': {'method': 'dbscan', 'eps': 1.0, 'min_samples': 3},
            'dbscan_tight': {'method': 'dbscan', 'eps': 0.5, 'min_samples': 5},
        }
        
        for name, params in methods.items():
            try:
                clusterer = BirdClusterer()
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
        """Get the best performing clustering method."""
        valid_results = {name: res for name, res in self.results.items() 
                        if 'error' not in res and 'metrics' in res}
        
        if not valid_results:
            return None, {}
        
        best_name = max(valid_results.keys(), 
                       key=lambda x: valid_results[x]['metrics'].get('silhouette_score', -2))
        
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
