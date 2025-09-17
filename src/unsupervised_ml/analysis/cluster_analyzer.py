"""
Analysis tools for evaluating bird clustering quality.
"""

import numpy as np
import pandas as pd
from sklearn.metrics.pairwise import cosine_similarity
from typing import Dict, List
import logging
from collections import Counter

class ClusterAnalyzer:
    """Advanced analysis tools for bird clustering evaluation."""
    
    def __init__(self, clusterer, features: np.ndarray, metadata: List[Dict]):
        self.clusterer = clusterer
        self.features = features
        self.metadata = metadata
        self.cluster_labels = clusterer.cluster_labels_
        
        logging.basicConfig(level=logging.INFO)
        self.logger = logging.getLogger(__name__)
        
        self.df = pd.DataFrame(metadata)
        self.df['cluster'] = self.cluster_labels
    
    def analyze_cluster_quality(self) -> Dict:
        """Comprehensive cluster quality analysis."""
        self.logger.info("📊 Analyzing cluster quality")
        
        results = {}
        
        # Basic statistics
        results['basic_stats'] = {
            'n_clusters': len(set(self.cluster_labels)) - (1 if -1 in self.cluster_labels else 0),
            'n_noise': list(self.cluster_labels).count(-1),
            'n_samples': len(self.cluster_labels),
            'cluster_sizes': dict(Counter(self.cluster_labels))
        }
        
        # Confidence analysis by cluster
        results['confidence_analysis'] = self._analyze_confidence_by_cluster()
        
        # Frame distribution analysis
        results['frame_analysis'] = self._analyze_frame_distribution()
        
        return results
    
    def _analyze_confidence_by_cluster(self) -> Dict:
        """Analyze YOLO confidence patterns by cluster."""
        cluster_confidence = self.df.groupby('cluster')['confidence'].agg([
            'count', 'mean', 'std', 'min', 'max'
        ]).round(3)
        
        return {
            'cluster_stats': cluster_confidence.to_dict(),
            'overall_confidence_mean': self.df['confidence'].mean()
        }
    
    def _analyze_frame_distribution(self) -> Dict:
        """Analyze how clusters are distributed across frames."""
        cluster_frame_diversity = {}
        for cluster in set(self.cluster_labels):
            if cluster != -1:
                cluster_frames = self.df[self.df['cluster'] == cluster]['frame_id'].nunique()
                total_objects = len(self.df[self.df['cluster'] == cluster])
                cluster_frame_diversity[cluster] = {
                    'unique_frames': cluster_frames,
                    'objects_per_frame': total_objects / cluster_frames if cluster_frames > 0 else 0
                }
        
        return {'cluster_frame_diversity': cluster_frame_diversity}
    
    def detect_potential_species_groups(self, confidence_threshold: float = 0.8) -> Dict:
        """Attempt to identify potential bird species based on clustering patterns."""
        self.logger.info("🐦 Analyzing potential species groups")
        
        high_conf_df = self.df[self.df['confidence'] >= confidence_threshold]
        
        if len(high_conf_df) == 0:
            return {'error': f'No detections above confidence threshold {confidence_threshold}'}
        
        species_analysis = {}
        
        for cluster in set(high_conf_df['cluster']):
            if cluster != -1:
                cluster_data = high_conf_df[high_conf_df['cluster'] == cluster]
                
                species_analysis[f'cluster_{cluster}'] = {
                    'sample_count': len(cluster_data),
                    'avg_confidence': cluster_data['confidence'].mean(),
                    'confidence_consistency': 1.0 - cluster_data['confidence'].std(),
                    'frame_distribution': cluster_data['frame_id'].nunique(),
                    'potential_species_score': self._calculate_species_score(cluster_data)
                }
        
        ranked_clusters = sorted(species_analysis.items(), 
                               key=lambda x: x[1].get('potential_species_score', 0), 
                               reverse=True)
        
        return {
            'high_confidence_samples': len(high_conf_df),
            'clusters_analyzed': len(species_analysis),
            'species_candidates': dict(ranked_clusters[:5])
        }
    
    def _calculate_species_score(self, cluster_data: pd.DataFrame) -> float:
        """Calculate a score indicating how likely a cluster represents a distinct species."""
        sample_score = min(len(cluster_data) / 10.0, 1.0)
        confidence_score = cluster_data['confidence'].mean()
        consistency_score = max(0, 1.0 - cluster_data['confidence'].std())
        frame_diversity_score = min(cluster_data['frame_id'].nunique() / 5.0, 1.0)
        
        species_score = (
            0.3 * sample_score +
            0.3 * confidence_score +
            0.2 * consistency_score +
            0.2 * frame_diversity_score
        )
        
        return species_score
    
    def analyze_visual_similarity_patterns(self) -> Dict:
        """Analyze visual similarity patterns within clusters."""
        self.logger.info("👁️ Analyzing visual similarity patterns")
        
        similarity_matrix = cosine_similarity(self.features)
        cluster_similarities = {}
        
        for cluster in set(self.cluster_labels):
            if cluster != -1:
                cluster_indices = np.where(self.cluster_labels == cluster)[0]
                
                if len(cluster_indices) > 1:
                    cluster_sim_matrix = similarity_matrix[np.ix_(cluster_indices, cluster_indices)]
                    mask = np.triu_indices_from(cluster_sim_matrix, k=1)
                    within_cluster_sims = cluster_sim_matrix[mask]
                    
                    cluster_similarities[cluster] = {
                        'mean_similarity': np.mean(within_cluster_sims),
                        'std_similarity': np.std(within_cluster_sims),
                    }
        
        cohesive_clusters = sorted(cluster_similarities.items(), 
                                 key=lambda x: x[1]['mean_similarity'], 
                                 reverse=True)
        
        return {
            'cluster_visual_cohesion': cluster_similarities,
            'most_cohesive_clusters': dict(cohesive_clusters[:3])
        }
    
    def generate_cluster_report(self) -> str:
        """Generate a comprehensive text report of cluster analysis."""
        self.logger.info("📝 Generating comprehensive cluster report")
        
        quality_analysis = self.analyze_cluster_quality()
        species_analysis = self.detect_potential_species_groups()
        similarity_analysis = self.analyze_visual_similarity_patterns()
        
        report_lines = [
            "🔬 BIRD CLUSTERING ANALYSIS REPORT",
            "=" * 50,
            "",
            f"📊 BASIC STATISTICS",
            f"Total objects analyzed: {quality_analysis['basic_stats']['n_samples']}",
            f"Clusters found: {quality_analysis['basic_stats']['n_clusters']}",
            f"Noise points: {quality_analysis['basic_stats']['n_noise']}",
            ""
        ]
        
        # Species analysis
        if 'species_candidates' in species_analysis:
            report_lines.extend([
                f"🐦 POTENTIAL SPECIES GROUPS",
                f"High-confidence samples: {species_analysis['high_confidence_samples']}",
                f"Clusters analyzed: {species_analysis['clusters_analyzed']}",
                "",
                "Top species candidates:"
            ])
            
            for i, (cluster_name, data) in enumerate(list(species_analysis['species_candidates'].items())[:3]):
                report_lines.append(
                    f"  {i+1}. {cluster_name}: {data['sample_count']} samples, "
                    f"score: {data['potential_species_score']:.3f}"
                )
        
        # Recommendations
        report_lines.extend([
            "",
            f"💡 RECOMMENDATIONS",
            f"• Consider clusters with high species scores as distinct bird types",
            f"• Investigate visually cohesive clusters for consistent characteristics",
            f"• Review noise points for potential outliers or rare species",
        ])
        
        return "\n".join(report_lines)
