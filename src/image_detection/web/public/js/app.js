/**
 * Birds of Play - Detection Results Web App
 * ========================================
 * 
 * Frontend JavaScript for viewing image detection results
 */

class DetectionResultsApp {
    constructor() {
        this.currentResults = [];
        this.currentSummary = null;
        this.modal = document.getElementById('detectionModal');
        this.originalFrameModal = document.getElementById('originalFrameModal');
        this.currentDetectionModel = null; // Store current detection model
        
        this.setupNavigation();
        this.initializeEventListeners();
        this.loadSummary();
        this.loadResults();
    }
    
    setupNavigation() {
        // Set up dynamic navigation links based on current port
        const currentPort = window.location.port;
        const motionDetectionPort = '3000';
        const detectionResultsPort = '3001';
        
        // Update navigation links to use the correct ports
        const motionLink = document.querySelector('.nav-link[href*="localhost"]');
        if (motionLink && currentPort === detectionResultsPort) {
            motionLink.href = `http://localhost:${motionDetectionPort}`;
        }
    }
    
    initializeEventListeners() {
        // Refresh button
        document.getElementById('refreshBtn').addEventListener('click', () => {
            this.loadSummary();
            this.loadResults();
        });
        
        // Model filter
        document.getElementById('modelFilter').addEventListener('change', (e) => {
            this.loadResults();
        });
        
        // Modal close buttons
        document.querySelectorAll('.close').forEach(closeBtn => {
            closeBtn.addEventListener('click', (e) => {
                const modal = e.target.closest('.modal');
                modal.style.display = 'none';
            });
        });
        
        // Close modals when clicking outside
        window.addEventListener('click', (e) => {
            if (e.target.classList.contains('modal')) {
                e.target.style.display = 'none';
            }
        });
        
        // View original frame button
        document.getElementById('viewOriginalBtn').addEventListener('click', () => {
            this.viewOriginalFrame();
        });
        
        // View detection image button
        document.getElementById('viewDetectionImageBtn').addEventListener('click', () => {
            this.viewDetectionImage();
        });
        
        // Detection overlay buttons
        document.getElementById('showDetectionOverlayBtn').addEventListener('click', () => {
            this.showDetectionOverlay();
        });
        
        document.getElementById('hideDetectionOverlayBtn').addEventListener('click', () => {
            this.hideDetectionOverlay();
        });
        
        // View original from detection button
        document.getElementById('viewOriginalFromDetectionBtn').addEventListener('click', () => {
            this.viewOriginalFromDetection();
        });
    }
    
    async loadSummary() {
        try {
            const response = await fetch('/api/detection-summary');
            const summary = await response.json();
            
            this.currentSummary = summary;
            this.updateSummaryDisplay(summary);
        } catch (error) {
            console.error('Error loading summary:', error);
            this.showError('Failed to load detection summary');
        }
    }
    
    async loadResults() {
        const loadingIndicator = document.getElementById('loadingIndicator');
        const resultsContainer = document.getElementById('resultsContainer');
        const modelFilter = document.getElementById('modelFilter');
        
        loadingIndicator.style.display = 'block';
        resultsContainer.innerHTML = '';
        
        try {
            const model = modelFilter.value;
            const response = await fetch(`/api/detection-results?model=${model}&limit=50`);
            const results = await response.json();
            
            this.currentResults = results;
            this.displayResults(results);
        } catch (error) {
            console.error('Error loading results:', error);
            this.showError('Failed to load detection results');
        } finally {
            loadingIndicator.style.display = 'none';
        }
    }
    
    updateSummaryDisplay(summary) {
        document.getElementById('totalResults').textContent = summary.totalResults || 0;
        document.getElementById('successRate').textContent = 
            summary.successRate ? `${(summary.successRate * 100).toFixed(1)}%` : '0%';
        document.getElementById('totalDetections').textContent = summary.totalDetections || 0;
        document.getElementById('avgDetections').textContent = 
            summary.avgDetectionsPerFrame ? summary.avgDetectionsPerFrame.toFixed(1) : '0';
    }
    
    displayResults(results) {
        const container = document.getElementById('resultsContainer');
        
        if (results.length === 0) {
            container.innerHTML = '<div class="no-results">No detection results found</div>';
            return;
        }
        
        container.innerHTML = results.map(result => this.createDetectionCard(result)).join('');
        
        // Add click listeners to cards
        container.querySelectorAll('.detection-card').forEach((card, index) => {
            card.addEventListener('click', () => {
                this.showDetectionDetails(results[index]);
            });
        });
    }
    
    createDetectionCard(result) {
        const processingTime = new Date(result.processing_timestamp).toLocaleString();
        const statusClass = result.processing_success ? 'status-success' : 'status-error';
        const statusText = result.processing_success ? 'Success' : 'Error';
        
        return `
            <div class="detection-card">
                <div class="detection-card-header">
                    <div class="detection-card-title">Frame ${result.frame_uuid.substring(0, 8)}...</div>
                    <div class="detection-status ${statusClass}">${statusText}</div>
                </div>
                
                <div class="detection-stats">
                    <div class="stat-item">
                        <div class="stat-label">Regions</div>
                        <div class="stat-value">${result.regions_processed}</div>
                    </div>
                    <div class="stat-item">
                        <div class="stat-label">Detections</div>
                        <div class="stat-value">${result.total_detections}</div>
                    </div>
                </div>
                
                <div class="detection-meta">
                    <div><strong>Model:</strong> ${result.detection_model}</div>
                    <div><strong>Processed:</strong> ${processingTime}</div>
                </div>
            </div>
        `;
    }
    
    showDetectionDetails(result) {
        // Store current detection model for later use
        this.currentDetectionModel = result.detection_model;
        
        // Update modal content
        document.getElementById('modalFrameUuid').textContent = result.frame_uuid;
        document.getElementById('modalProcessingTime').textContent = 
            new Date(result.processing_timestamp).toLocaleString();
        document.getElementById('modalDetectionModel').textContent = result.detection_model;
        document.getElementById('modalRegionsProcessed').textContent = result.regions_processed;
        document.getElementById('modalTotalDetections').textContent = result.total_detections;
        
        // Display detections
        this.displayDetections(result.region_results);
        
        // Show modal
        this.modal.style.display = 'block';
    }
    
    displayDetections(regionResults) {
        const container = document.getElementById('modalDetections');
        
        if (!regionResults || regionResults.length === 0) {
            container.innerHTML = '<div class="no-detections">No detections found</div>';
            return;
        }
        
        let allDetections = [];
        regionResults.forEach(region => {
            if (region.detections && region.detections.length > 0) {
                allDetections = allDetections.concat(region.detections);
            }
        });
        
        if (allDetections.length === 0) {
            container.innerHTML = '<div class="no-detections">No objects detected</div>';
            return;
        }
        
        container.innerHTML = allDetections.map(detection => `
            <div class="detection-item">
                <div class="detection-item-header">
                    <div class="detection-class">${detection.class_name}</div>
                    <div class="detection-confidence">${(detection.confidence * 100).toFixed(1)}%</div>
                </div>
                <div class="detection-coords">
                    <div class="coord-item">
                        <span>Full Frame:</span>
                        <span>(${detection.bbox_x1}, ${detection.bbox_y1}) → (${detection.bbox_x2}, ${detection.bbox_y2})</span>
                    </div>
                    <div class="coord-item">
                        <span>Region:</span>
                        <span>(${detection.region_x}, ${detection.region_y}) ${detection.region_w}×${detection.region_h}</span>
                    </div>
                </div>
            </div>
        `).join('');
    }
    
    async viewOriginalFrame() {
        const frameUuid = document.getElementById('modalFrameUuid').textContent;
        
        try {
            const response = await fetch(`/api/original-frame/${frameUuid}`);
            const frame = await response.json();
            
            // Update original frame modal
            document.getElementById('originalFrameUuid').textContent = frame.uuid;
            document.getElementById('originalFrameTimestamp').textContent = 
                new Date(frame.timestamp).toLocaleString();
            
            const regions = frame.metadata.consolidated_regions || [];
            document.getElementById('originalFrameRegions').textContent = regions.length;
            
            // Set image source to original frame
            const imageUrl = `/api/original-frame-image/${frameUuid}`;
            document.getElementById('originalFrameImage').src = imageUrl;
            
            // Close detection modal and show original frame modal
            this.modal.style.display = 'none';
            this.originalFrameModal.style.display = 'block';
            
        } catch (error) {
            console.error('Error loading original frame:', error);
            this.showError('Failed to load original frame');
        }
    }
    
    async viewDetectionImage() {
        const frameUuid = document.getElementById('modalFrameUuid').textContent;
        const model = document.getElementById('modalDetectionModel').textContent;
        const totalDetections = document.getElementById('modalTotalDetections').textContent;
        
        try {
            const imageUrl = `/api/detection-image/${frameUuid}?model=${model}`;
            
            // Update detection image modal
            document.getElementById('detectionImageUuid').textContent = frameUuid;
            document.getElementById('detectionImageModel').textContent = model;
            document.getElementById('detectionImageCount').textContent = totalDetections;
            
            // Set image source
            document.getElementById('detectionImage').src = imageUrl;
            
            // Close detection modal and show detection image modal
            this.modal.style.display = 'none';
            document.getElementById('detectionImageModal').style.display = 'block';
            
        } catch (error) {
            console.error('Error loading detection image:', error);
            this.showError('Failed to load detection image');
        }
    }
    
    showDetectionOverlay() {
        const frameUuid = document.getElementById('originalFrameUuid').textContent;
        const model = this.currentDetectionModel;
        
        if (!model) {
            this.showError('No detection model available');
            return;
        }
        
        // Update the original frame image to show detection overlays
        const imageUrl = `/api/detection-image/${frameUuid}?model=${model}`;
        document.getElementById('originalFrameImage').src = imageUrl;
        
        // Toggle button visibility
        document.getElementById('showDetectionOverlayBtn').style.display = 'none';
        document.getElementById('hideDetectionOverlayBtn').style.display = 'inline-block';
    }
    
    hideDetectionOverlay() {
        const frameUuid = document.getElementById('originalFrameUuid').textContent;
        
        // Restore the original frame image without overlays
        const imageUrl = `/api/original-frame-image/${frameUuid}`;
        document.getElementById('originalFrameImage').src = imageUrl;
        
        // Toggle button visibility
        document.getElementById('showDetectionOverlayBtn').style.display = 'inline-block';
        document.getElementById('hideDetectionOverlayBtn').style.display = 'none';
    }
    
    viewOriginalFromDetection() {
        // Close detection image modal and show original frame modal
        document.getElementById('detectionImageModal').style.display = 'none';
        this.originalFrameModal.style.display = 'block';
    }
    
    showError(message) {
        // Simple error display - could be enhanced with a proper notification system
        alert(`Error: ${message}`);
    }
}

// Initialize the app when the page loads
document.addEventListener('DOMContentLoaded', () => {
    new DetectionResultsApp();
});
