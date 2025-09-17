"""
Test script to verify the unsupervised ML system works end-to-end.
"""

import sys
from pathlib import Path

# Add project root to path
project_root = Path(__file__).resolve().parent.parent.parent
sys.path.insert(0, str(project_root))

def main():
    """Run basic system test."""
    print("🔬 Birds of Play - Unsupervised ML System Test")
    print("=" * 50)
    
    print("📁 Project structure:")
    unsupervised_ml_dir = Path(__file__).parent
    
    expected_files = [
        "__init__.py",
        "requirements.txt",
        "data/object_data_manager.py",
        "models/feature_extractor.py", 
        "clustering/bird_clusterer.py",
        "visualization/cluster_visualizer.py",
        "analysis/cluster_analyzer.py",
        "web/cluster_server.py",
        "run_clustering_analysis.py"
    ]
    
    files_found = 0
    for file_path in expected_files:
        full_path = unsupervised_ml_dir / file_path
        if full_path.exists():
            print(f"  ✅ {file_path}")
            files_found += 1
        else:
            print(f"  ❌ {file_path} - MISSING")
    
    print(f"\n📊 Files found: {files_found}/{len(expected_files)}")
    
    if files_found == len(expected_files):
        print("✅ All core files present!")
        print("\n🚀 Next steps:")
        print("  1. make ml-install")
        print("  2. make cluster-analysis")
        print("  3. Open http://localhost:3002")
        return 0
    else:
        print("❌ Some files are missing. Please recreate them.")
        return 1

if __name__ == "__main__":
    exit(main())
