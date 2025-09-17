"""
Interactive visualization for bird clustering results.
"""

import numpy as np
import pandas as pd
import plotly.express as px
import plotly.graph_objects as go
from typing import Dict, List
import json
import logging
from pathlib import Path

class ClusterVisualizer:
    """Create interactive visualizations for bird clustering results."""
    
    def __init__(self, clusterer, output_dir: str = "data/visualizations"):
        self.clusterer = clusterer
        self.output_dir = Path(output_dir)
        self.output_dir.mkdir(parents=True, exist_ok=True)
        
        logging.basicConfig(level=logging.INFO)
        self.logger = logging.getLogger(__name__)
        
        if self.clusterer.cluster_labels_ is None:
            raise ValueError("Clusterer must be fitted before visualization")
    
    def create_scatter_plot(self, title: str = "Bird Clustering Results", 
                           save_html: bool = True) -> go.Figure:
        """Create interactive scatter plot of clustering results."""
        if self.clusterer.features_2d_ is None:
            raise ValueError("2D features not available. Run clustering first.")
        
        df = pd.DataFrame({
            'x': self.clusterer.features_2d_[:, 0],
            'y': self.clusterer.features_2d_[:, 1],
            'cluster': self.clusterer.cluster_labels_,
            'object_id': [meta['object_id'] for meta in self.clusterer.metadata_],
            'confidence': [meta['confidence'] for meta in self.clusterer.metadata_],
        })
        
        fig = px.scatter(
            df, x='x', y='y', 
            color='cluster',
            hover_data=['object_id', 'confidence'],
            title=title,
            labels={'x': 'Dimension 1', 'y': 'Dimension 2'},
        )
        
        fig.update_layout(width=800, height=600, title_x=0.5)
        
        if save_html:
            html_path = self.output_dir / "clustering_scatter.html"
            fig.write_html(str(html_path))
            self.logger.info(f"Scatter plot saved to {html_path}")
        
        return fig
    
    
    
    def save_all_visualizations(self, similarity_threshold: float = 0.8):
        """Create and save all visualizations."""
        self.logger.info("Creating all visualizations...")
        
        paths = {}
        
        try:
            scatter_fig = self.create_scatter_plot()
            scatter_path = self.output_dir / "clustering_scatter.html"
            scatter_fig.write_html(str(scatter_path))
            paths['scatter'] = str(scatter_path)
        except Exception as e:
            self.logger.error(f"Failed to create scatter plot: {e}")
        
        
        self.logger.info(f"Visualizations saved to {self.output_dir}")
        
        return paths
