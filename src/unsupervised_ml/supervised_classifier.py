"""
Supervised Bird Classifier using SimCLR backbone with clustering pseudo-labels.
"""

import torch
import torch.nn as nn
import torch.optim as optim
from torch.utils.data import Dataset, DataLoader
import torchvision.transforms as transforms
from torchvision.models import resnet50, resnet18
import numpy as np
import pandas as pd
from PIL import Image
from pathlib import Path
from typing import Dict, List, Tuple, Optional
import logging
from sklearn.model_selection import train_test_split
from sklearn.metrics import accuracy_score, classification_report, confusion_matrix
import json
from config_loader import load_clustering_config, ClusteringConfig

class BirdDataset(Dataset):
    """Dataset for bird images with cluster pseudo-labels."""
    
    def __init__(self, image_paths: List[str], labels: List[int], 
                 transform: Optional[transforms.Compose] = None):
        self.image_paths = image_paths
        self.labels = labels
        self.transform = transform
        
    def __len__(self):
        return len(self.image_paths)
    
    def __getitem__(self, idx):
        image_path = self.image_paths[idx]
        label = self.labels[idx]
        
        # Load image
        try:
            image = Image.open(image_path).convert('RGB')
        except Exception as e:
            # Return a black image if loading fails
            image = Image.new('RGB', (224, 224), color='black')
            
        if self.transform:
            image = self.transform(image)
            
        return image, label


class SimCLRBackbone(nn.Module):
    """SimCLR-style backbone using ResNet."""
    
    def __init__(self, model_name: str = 'resnet50', pretrained: bool = True):
        super().__init__()
        
        if model_name == 'resnet50':
            self.backbone = resnet50(pretrained=pretrained)
            self.feature_dim = 2048
        elif model_name == 'resnet18':
            self.backbone = resnet18(pretrained=pretrained)
            self.feature_dim = 512
        else:
            raise ValueError(f"Unsupported model: {model_name}")
            
        # Remove the final classification layer
        self.backbone = nn.Sequential(*list(self.backbone.children())[:-1])
        
    def forward(self, x):
        features = self.backbone(x)
        features = features.view(features.size(0), -1)  # Flatten
        return features


class BirdClassifier(nn.Module):
    """Bird classifier with SimCLR backbone and classification head."""
    
    def __init__(self, n_classes: int, model_name: str = 'resnet50', 
                 pretrained: bool = True, dropout_rate: float = 0.5):
        super().__init__()
        
        self.backbone = SimCLRBackbone(model_name, pretrained)
        self.feature_dim = self.backbone.feature_dim
        
        # Classification head
        self.classifier = nn.Sequential(
            nn.Dropout(dropout_rate),
            nn.Linear(self.feature_dim, 512),
            nn.ReLU(),
            nn.Dropout(dropout_rate),
            nn.Linear(512, n_classes)
        )
        
        self.n_classes = n_classes
        
    def forward(self, x):
        features = self.backbone(x)
        logits = self.classifier(features)
        return logits
    
    def get_features(self, x):
        """Extract features without classification."""
        return self.backbone(x)
    
    def freeze_backbone(self):
        """Freeze backbone parameters for initial training."""
        for param in self.backbone.parameters():
            param.requires_grad = False
            
    def unfreeze_backbone(self):
        """Unfreeze backbone parameters for fine-tuning."""
        for param in self.backbone.parameters():
            param.requires_grad = True


class SupervisedBirdTrainer:
    """Trainer for supervised bird classification using clustering pseudo-labels."""
    
    def __init__(self, config: Optional[ClusteringConfig] = None):
        self.config = config if config is not None else load_clustering_config()
        
        # Set up logging
        log_level = getattr(logging, self.config.log_level.upper(), logging.INFO)
        logging.basicConfig(level=log_level)
        self.logger = logging.getLogger(__name__)
        
        # Device setup
        self.device = torch.device('cuda' if torch.cuda.is_available() and self.config.use_gpu else 'cpu')
        self.logger.info(f"Using device: {self.device}")
        
        # Model and training components
        self.model = None
        self.train_loader = None
        self.val_loader = None
        self.test_loader = None
        self.optimizer = None
        self.scheduler = None
        self.criterion = nn.CrossEntropyLoss()
        
        # Training history
        self.train_history = {
            'train_loss': [],
            'train_acc': [],
            'val_loss': [],
            'val_acc': []
        }
        
    def prepare_data_from_clustering(self, clusterer, metadata: List[Dict], 
                                   test_size: float = 0.2, val_size: float = 0.1) -> bool:
        """
        Prepare training data using clustering results as pseudo-labels.
        
        Args:
            clusterer: Fitted BirdClusterer instance
            metadata: Bird object metadata
            test_size: Fraction for test set
            val_size: Fraction for validation set
            
        Returns:
            True if successful, False otherwise
        """
        if clusterer.cluster_labels_ is None:
            self.logger.error("Clusterer must be fitted first")
            return False
            
        # Extract image paths and cluster labels
        image_paths = []
        labels = []
        object_ids = []
        
        project_root = Path(__file__).parent.parent.parent
        objects_dir = project_root / "data" / "objects"
        
        for i, meta in enumerate(metadata):
            object_id = meta['object_id']
            image_path = objects_dir / f"{object_id}.jpg"
            
            if image_path.exists():
                image_paths.append(str(image_path))
                labels.append(int(clusterer.cluster_labels_[i]))
                object_ids.append(object_id)
        
        if len(image_paths) == 0:
            self.logger.error("No valid image paths found")
            return False
            
        self.logger.info(f"Found {len(image_paths)} images with {len(set(labels))} clusters")
        
        # Create model with correct number of classes
        n_classes = len(set(labels))
        self.model = BirdClassifier(
            n_classes=n_classes,
            model_name=self.config.model_name,
            pretrained=True
        ).to(self.device)
        
        self.logger.info(f"Created model with {n_classes} classes")
        
        # Check if stratified split is possible (each class needs at least 2 samples)
        label_counts = {label: labels.count(label) for label in set(labels)}
        min_class_size = min(label_counts.values())
        can_stratify = min_class_size >= 2
        
        self.logger.info(f"Label distribution: {label_counts}, min_class_size: {min_class_size}")
        
        # Split data
        if len(image_paths) < 6 or not can_stratify:
            # Too few samples or unbalanced classes - use simple split
            self.logger.info("Using simple train/val/test split (no stratification)")
            
            # Simple random split
            indices = list(range(len(image_paths)))
            np.random.seed(42)
            np.random.shuffle(indices)
            
            n_test = max(1, int(len(indices) * test_size))
            n_val = max(1, int(len(indices) * val_size))
            n_train = len(indices) - n_test - n_val
            
            train_indices = indices[:n_train]
            val_indices = indices[n_train:n_train+n_val]
            test_indices = indices[n_train+n_val:]
            
            train_paths = [image_paths[i] for i in train_indices]
            train_labels = [labels[i] for i in train_indices]
            val_paths = [image_paths[i] for i in val_indices]
            val_labels = [labels[i] for i in val_indices]
            test_paths = [image_paths[i] for i in test_indices]
            test_labels = [labels[i] for i in test_indices]
            
        else:
            # Stratified split
            self.logger.info("Using stratified train/val/test split")
            train_paths, test_paths, train_labels, test_labels = train_test_split(
                image_paths, labels, test_size=test_size, stratify=labels, random_state=42
            )
            
            # Check if we can do stratified split for validation
            train_label_counts = {label: train_labels.count(label) for label in set(train_labels)}
            can_stratify_val = min(train_label_counts.values()) >= 2
            
            if len(train_paths) >= 4 and can_stratify_val:
                train_paths, val_paths, train_labels, val_labels = train_test_split(
                    train_paths, train_labels, test_size=val_size/(1-test_size), 
                    stratify=train_labels, random_state=42
                )
            else:
                # Simple validation split
                n_val = max(1, min(2, len(train_paths) // 2))
                val_paths = train_paths[:n_val]
                val_labels = train_labels[:n_val]
                train_paths = train_paths[n_val:]
                train_labels = train_labels[n_val:]
        
        # Data transforms
        train_transform = transforms.Compose([
            transforms.Resize((224, 224)),
            transforms.RandomHorizontalFlip(p=0.5),
            transforms.RandomRotation(degrees=15),
            transforms.ColorJitter(brightness=0.2, contrast=0.2, saturation=0.2, hue=0.1),
            transforms.ToTensor(),
            transforms.Normalize(mean=[0.485, 0.456, 0.406], std=[0.229, 0.224, 0.225])
        ])
        
        val_transform = transforms.Compose([
            transforms.Resize((224, 224)),
            transforms.ToTensor(),
            transforms.Normalize(mean=[0.485, 0.456, 0.406], std=[0.229, 0.224, 0.225])
        ])
        
        # Create datasets and dataloaders
        train_dataset = BirdDataset(train_paths, train_labels, train_transform)
        val_dataset = BirdDataset(val_paths, val_labels, val_transform)
        test_dataset = BirdDataset(test_paths, test_labels, val_transform)
        
        batch_size = min(self.config.batch_size, len(train_dataset))
        
        self.train_loader = DataLoader(train_dataset, batch_size=batch_size, shuffle=True, num_workers=2)
        self.val_loader = DataLoader(val_dataset, batch_size=batch_size, shuffle=False, num_workers=2)
        self.test_loader = DataLoader(test_dataset, batch_size=batch_size, shuffle=False, num_workers=2)
        
        self.logger.info(f"Data splits - Train: {len(train_dataset)}, Val: {len(val_dataset)}, Test: {len(test_dataset)}")
        
        # Store metadata for later use
        self.cluster_to_label_map = {i: f"Species_{i}" for i in range(n_classes)}
        self.training_metadata = {
            'n_classes': n_classes,
            'train_size': len(train_dataset),
            'val_size': len(val_dataset),
            'test_size': len(test_dataset),
            'cluster_distribution': {str(label): labels.count(label) for label in set(labels)}
        }
        
        return True
    
    def train_phase1_frozen_backbone(self, epochs: int = 10, lr: float = 0.001):
        """Phase 1: Train with frozen backbone."""
        self.logger.info("Phase 1: Training with frozen backbone")
        
        # Freeze backbone
        self.model.freeze_backbone()
        
        # Setup optimizer for classifier only
        classifier_params = [p for p in self.model.classifier.parameters() if p.requires_grad]
        self.optimizer = optim.Adam(classifier_params, lr=lr, weight_decay=1e-4)
        self.scheduler = optim.lr_scheduler.StepLR(self.optimizer, step_size=5, gamma=0.5)
        
        # Train
        for epoch in range(epochs):
            train_loss, train_acc = self._train_epoch()
            val_loss, val_acc = self._validate()
            
            self.train_history['train_loss'].append(train_loss)
            self.train_history['train_acc'].append(train_acc)
            self.train_history['val_loss'].append(val_loss)
            self.train_history['val_acc'].append(val_acc)
            
            self.scheduler.step()
            
            self.logger.info(f"Epoch {epoch+1}/{epochs} - "
                           f"Train Loss: {train_loss:.4f}, Train Acc: {train_acc:.4f}, "
                           f"Val Loss: {val_loss:.4f}, Val Acc: {val_acc:.4f}")
    
    def train_phase2_full_finetuning(self, epochs: int = 5, lr: float = 0.0001):
        """Phase 2: Fine-tune entire model."""
        self.logger.info("Phase 2: Fine-tuning entire model")
        
        # Unfreeze backbone
        self.model.unfreeze_backbone()
        
        # Setup optimizer for all parameters
        self.optimizer = optim.Adam(self.model.parameters(), lr=lr, weight_decay=1e-4)
        self.scheduler = optim.lr_scheduler.StepLR(self.optimizer, step_size=3, gamma=0.5)
        
        # Train
        for epoch in range(epochs):
            train_loss, train_acc = self._train_epoch()
            val_loss, val_acc = self._validate()
            
            self.train_history['train_loss'].append(train_loss)
            self.train_history['train_acc'].append(train_acc)
            self.train_history['val_loss'].append(val_loss)
            self.train_history['val_acc'].append(val_acc)
            
            self.scheduler.step()
            
            self.logger.info(f"Fine-tune Epoch {epoch+1}/{epochs} - "
                           f"Train Loss: {train_loss:.4f}, Train Acc: {train_acc:.4f}, "
                           f"Val Loss: {val_loss:.4f}, Val Acc: {val_acc:.4f}")
    
    def _train_epoch(self) -> Tuple[float, float]:
        """Train for one epoch."""
        self.model.train()
        total_loss = 0.0
        correct = 0
        total = 0
        
        for batch_idx, (data, target) in enumerate(self.train_loader):
            data, target = data.to(self.device), target.to(self.device)
            
            self.optimizer.zero_grad()
            output = self.model(data)
            loss = self.criterion(output, target)
            loss.backward()
            self.optimizer.step()
            
            total_loss += loss.item()
            pred = output.argmax(dim=1, keepdim=True)
            correct += pred.eq(target.view_as(pred)).sum().item()
            total += target.size(0)
        
        avg_loss = total_loss / len(self.train_loader)
        accuracy = correct / total
        
        return avg_loss, accuracy
    
    def _validate(self) -> Tuple[float, float]:
        """Validate model."""
        self.model.eval()
        total_loss = 0.0
        correct = 0
        total = 0
        
        with torch.no_grad():
            for data, target in self.val_loader:
                data, target = data.to(self.device), target.to(self.device)
                output = self.model(data)
                loss = self.criterion(output, target)
                
                total_loss += loss.item()
                pred = output.argmax(dim=1, keepdim=True)
                correct += pred.eq(target.view_as(pred)).sum().item()
                total += target.size(0)
        
        avg_loss = total_loss / len(self.val_loader)
        accuracy = correct / total
        
        return avg_loss, accuracy
    
    def evaluate(self) -> Dict:
        """Evaluate model on test set."""
        self.model.eval()
        predictions = []
        targets = []
        confidences = []
        
        with torch.no_grad():
            for data, target in self.test_loader:
                data, target = data.to(self.device), target.to(self.device)
                output = self.model(data)
                
                # Get predictions and confidence scores
                probs = torch.softmax(output, dim=1)
                pred = output.argmax(dim=1)
                conf = probs.max(dim=1)[0]
                
                predictions.extend(pred.cpu().numpy())
                targets.extend(target.cpu().numpy())
                confidences.extend(conf.cpu().numpy())
        
        accuracy = accuracy_score(targets, predictions)
        
        # Classification report - handle case where test set doesn't contain all classes
        unique_labels = sorted(set(targets + predictions))
        class_names = [self.cluster_to_label_map[i] for i in unique_labels]
        
        try:
            report = classification_report(targets, predictions, 
                                         labels=unique_labels, 
                                         target_names=class_names, 
                                         output_dict=True, 
                                         zero_division=0)
        except Exception as e:
            self.logger.warning(f"Classification report failed: {e}")
            report = {'accuracy': accuracy}
        
        self.logger.info(f"Test Accuracy: {accuracy:.4f}")
        
        return {
            'accuracy': accuracy,
            'classification_report': report,
            'predictions': predictions,
            'targets': targets,
            'confidences': confidences,
            'mean_confidence': np.mean(confidences),
            'low_confidence_threshold': np.percentile(confidences, 20)  # Bottom 20%
        }
    
    def predict_with_confidence(self, image_paths: List[str], top_k: int = 3) -> List[Dict]:
        """
        Make predictions with confidence scores and top-k results.
        
        Args:
            image_paths: List of image file paths
            top_k: Number of top predictions to return
            
        Returns:
            List of prediction dictionaries
        """
        self.model.eval()
        
        # Transform for inference
        transform = transforms.Compose([
            transforms.Resize((224, 224)),
            transforms.ToTensor(),
            transforms.Normalize(mean=[0.485, 0.456, 0.406], std=[0.229, 0.224, 0.225])
        ])
        
        results = []
        
        with torch.no_grad():
            for image_path in image_paths:
                try:
                    # Load and preprocess image
                    image = Image.open(image_path).convert('RGB')
                    image_tensor = transform(image).unsqueeze(0).to(self.device)
                    
                    # Get prediction
                    output = self.model(image_tensor)
                    probs = torch.softmax(output, dim=1)
                    
                    # Get top-k predictions (limited by number of classes)
                    actual_k = min(top_k, probs.size(1))
                    top_probs, top_indices = torch.topk(probs, actual_k, dim=1)
                    
                    predictions = []
                    for i in range(actual_k):
                        class_idx = top_indices[0][i].item()
                        confidence = top_probs[0][i].item()
                        class_name = self.cluster_to_label_map[class_idx]
                        
                        predictions.append({
                            'class_id': class_idx,
                            'class_name': class_name,
                            'confidence': confidence
                        })
                    
                    results.append({
                        'image_path': image_path,
                        'predictions': predictions,
                        'max_confidence': predictions[0]['confidence']
                    })
                    
                except Exception as e:
                    self.logger.error(f"Error processing {image_path}: {e}")
                    results.append({
                        'image_path': image_path,
                        'error': str(e),
                        'predictions': [],
                        'max_confidence': 0.0
                    })
        
        return results
    
    def save_model(self, filepath: str):
        """Save trained model."""
        torch.save({
            'model_state_dict': self.model.state_dict(),
            'training_metadata': self.training_metadata,
            'cluster_to_label_map': self.cluster_to_label_map,
            'train_history': self.train_history,
            'config': {
                'n_classes': self.model.n_classes,
                'model_name': self.config.model_name,
                'feature_dim': self.model.feature_dim
            }
        }, filepath)
        
        self.logger.info(f"Model saved to {filepath}")
    
    def load_model(self, filepath: str):
        """Load trained model."""
        checkpoint = torch.load(filepath, map_location=self.device)
        
        # Recreate model
        config_data = checkpoint['config']
        self.model = BirdClassifier(
            n_classes=config_data['n_classes'],
            model_name=self.config.model_name
        ).to(self.device)
        
        # Load state dict
        self.model.load_state_dict(checkpoint['model_state_dict'])
        
        # Restore metadata
        self.training_metadata = checkpoint['training_metadata']
        self.cluster_to_label_map = checkpoint['cluster_to_label_map']
        self.train_history = checkpoint['train_history']
        
        self.logger.info(f"Model loaded from {filepath}")


# Example usage and testing
if __name__ == "__main__":
    # This would be called after clustering is complete
    print("🧠 Supervised Bird Classifier")
    print("Use this after running clustering to train a classifier with pseudo-labels")
    print("")
    print("Example usage:")
    print("1. Run clustering: python -c 'from bird_clusterer import ClusteringExperiment; ...'")
    print("2. Train classifier: python supervised_classifier.py")
    print("3. Use for inference and active learning")
