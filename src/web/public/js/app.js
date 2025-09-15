// Global variables
let currentPage = 1;
let currentSort = 'timestamp-desc';
let searchQuery = '';
let allFrames = [];

// DOM elements
const framesContainer = document.getElementById('framesContainer');
const loading = document.getElementById('loading');
const error = document.getElementById('error');
const errorMessage = document.getElementById('errorMessage');
const totalFrames = document.getElementById('totalFrames');
const lastUpdate = document.getElementById('lastUpdate');
const searchInput = document.getElementById('searchInput');
const searchBtn = document.getElementById('searchBtn');
const sortSelect = document.getElementById('sortSelect');
const refreshBtn = document.getElementById('refreshBtn');
const pagination = document.getElementById('pagination');
const prevPage = document.getElementById('prevPage');
const nextPage = document.getElementById('nextPage');
const pageInfo = document.getElementById('pageInfo');
const imageModal = document.getElementById('imageModal');
const modalImage = document.getElementById('modalImage');
const modalTitle = document.getElementById('modalTitle');
const modalUuid = document.getElementById('modalUuid');
const modalFilename = document.getElementById('modalFilename');
const modalTimestamp = document.getElementById('modalTimestamp');
const modalSource = document.getElementById('modalSource');
const modalAutoSaved = document.getElementById('modalAutoSaved');
const closeModal = document.querySelector('.close');

// Initialize the application
document.addEventListener('DOMContentLoaded', function() {
    loadFrames();
    setupEventListeners();
    updateLastUpdate();
});

// Setup event listeners
function setupEventListeners() {
    searchBtn.addEventListener('click', handleSearch);
    searchInput.addEventListener('keypress', function(e) {
        if (e.key === 'Enter') handleSearch();
    });
    
    sortSelect.addEventListener('change', function() {
        currentSort = this.value;
        currentPage = 1;
        loadFrames();
    });
    
    refreshBtn.addEventListener('click', function() {
        currentPage = 1;
        searchQuery = '';
        searchInput.value = '';
        loadFrames();
    });
    
    prevPage.addEventListener('click', function() {
        if (currentPage > 1) {
            currentPage--;
            loadFrames();
        }
    });
    
    nextPage.addEventListener('click', function() {
        currentPage++;
        loadFrames();
    });
    
    // Modal events
    closeModal.addEventListener('click', closeImageModal);
    window.addEventListener('click', function(e) {
        if (e.target === imageModal) {
            closeImageModal();
        }
    });
    
    // Auto-refresh every 30 seconds
    setInterval(function() {
        if (!searchQuery) { // Only auto-refresh if not searching
            loadFrames();
        }
    }, 30000);
}

// Load frames from API
async function loadFrames() {
    showLoading();
    hideError();
    
    try {
        const response = await fetch(`/api/frames?page=${currentPage}&limit=20`);
        const data = await response.json();
        
        if (response.ok) {
            allFrames = data.frames;
            displayFrames(allFrames);
            updatePagination(data.pagination);
            updateStats(data.pagination.total);
        } else {
            throw new Error(data.error || 'Failed to load frames');
        }
    } catch (err) {
        showError(err.message);
    } finally {
        hideLoading();
    }
}

// Display frames in the grid
function displayFrames(frames) {
    const infoBanner = document.getElementById('infoBanner');
    
    if (frames.length === 0) {
        framesContainer.innerHTML = `
            <div class="no-frames">
                <i class="fas fa-images"></i>
                <h3>No frames found</h3>
                <p>No frames match your search criteria or the database is empty.</p>
            </div>
        `;
        infoBanner.style.display = 'none';
        return;
    }
    
    // Check if any frames have image data
    const hasImageData = frames.some(frame => frame.image_data && frame.image_data.length > 0);
    
    if (!hasImageData) {
        infoBanner.style.display = 'flex';
    } else {
        infoBanner.style.display = 'none';
    }
    
    framesContainer.innerHTML = frames.map(frame => createFrameCard(frame)).join('');
}

// Create a frame card element
function createFrameCard(frame) {
    // Parse timestamp - handle both Unix timestamp (C++) and ISO format (Python)
    let timestamp = 'Unknown';
    if (frame.metadata.timestamp) {
        // Try Unix timestamp first (C++ format - string of seconds)
        const timestampNum = parseInt(frame.metadata.timestamp);
        if (!isNaN(timestampNum)) {
            // Convert seconds to milliseconds for JavaScript Date
            timestamp = new Date(timestampNum * 1000).toLocaleString();
        } else {
            // Try ISO format (Python format - ISO string)
            const timestampDate = new Date(frame.metadata.timestamp);
            if (!isNaN(timestampDate.getTime())) {
                timestamp = timestampDate.toLocaleString();
            }
        }
    }
    const uuid = frame._id || 'N/A';
    const filename = frame.metadata.original_filename || 'Unknown';
    const source = frame.metadata.source || 'Unknown';
    const autoSaved = frame.metadata.auto_saved ? 'Yes' : 'No';
    const motionDetected = frame.metadata.motion_detected ? 'Yes' : 'No';
    const motionRegions = frame.metadata.motion_regions || 0;
    const confidence = frame.metadata.confidence || 0;
    
    // Check if image data exists
    const hasImageData = frame.image_data && frame.image_data.length > 0;
    const imageSrc = hasImageData ? `data:image/jpeg;base64,${frame.image_data}` : '/images/placeholder.svg';
    
    return `
        <div class="frame-card" onclick="openImageModal('${uuid}')">
            <div class="frame-image-container">
                <img src="${imageSrc}" alt="${filename}" class="frame-image" 
                     onerror="this.src='/images/placeholder.svg'">
                ${!hasImageData ? '<div class="no-image-overlay"><i class="fas fa-image"></i><span>No Image Data</span></div>' : ''}
            </div>
            <div class="frame-info">
                <div class="frame-title">${filename}</div>
                <div class="frame-meta">
                    <div class="meta-item">
                        <span class="meta-label">UUID:</span>
                        <span class="meta-value" title="${uuid}">${uuid.substring(0, 8)}...</span>
                    </div>
                    <div class="meta-item">
                        <span class="meta-label">Timestamp:</span>
                        <span class="meta-value">${timestamp}</span>
                    </div>
                    <div class="meta-item">
                        <span class="meta-label">Source:</span>
                        <span class="meta-value">${source}</span>
                    </div>
                    <div class="meta-item">
                        <span class="meta-label">Motion:</span>
                        <span class="meta-value">${motionDetected}</span>
                    </div>
                    ${motionRegions > 0 ? `
                    <div class="meta-item">
                        <span class="meta-label">Regions:</span>
                        <span class="meta-value">${motionRegions}</span>
                    </div>
                    ` : ''}
                    ${confidence > 0 ? `
                    <div class="meta-item">
                        <span class="meta-label">Confidence:</span>
                        <span class="meta-value">${(confidence * 100).toFixed(1)}%</span>
                    </div>
                    ` : ''}
                </div>
            </div>
        </div>
    `;
}

// Handle search
function handleSearch() {
    searchQuery = searchInput.value.trim();
    currentPage = 1;
    
    if (searchQuery) {
        // Filter frames locally for better performance
        const filteredFrames = allFrames.filter(frame => {
            const uuid = frame._id || '';
            const filename = frame.metadata.original_filename || '';
            const searchLower = searchQuery.toLowerCase();
            
            return uuid.toLowerCase().includes(searchLower) || 
                   filename.toLowerCase().includes(searchLower);
        });
        
        displayFrames(filteredFrames);
        updatePagination({ page: 1, total: filteredFrames.length, pages: 1 });
        updateStats(filteredFrames.length);
    } else {
        loadFrames();
    }
}

// Update pagination controls
function updatePagination(paginationData) {
    if (paginationData.pages <= 1) {
        pagination.style.display = 'none';
        return;
    }
    
    pagination.style.display = 'flex';
    pageInfo.textContent = `Page ${paginationData.page} of ${paginationData.pages}`;
    
    prevPage.disabled = paginationData.page <= 1;
    nextPage.disabled = paginationData.page >= paginationData.pages;
    
    prevPage.style.opacity = prevPage.disabled ? '0.5' : '1';
    nextPage.style.opacity = nextPage.disabled ? '0.5' : '1';
}

// Update statistics
function updateStats(total) {
    totalFrames.textContent = total;
}

// Update last update time
function updateLastUpdate() {
    const now = new Date();
    lastUpdate.textContent = now.toLocaleTimeString();
}

// Open image modal
async function openImageModal(uuid) {
    try {
        const response = await fetch(`/api/frames/${uuid}`);
        const frame = await response.json();
        
        if (response.ok) {
            const hasImageData = frame.image_data && frame.image_data.length > 0;
            const imageSrc = hasImageData ? `data:image/jpeg;base64,${frame.image_data}` : '/images/placeholder.svg';
            
            modalImage.src = imageSrc;
            modalTitle.textContent = frame.metadata.original_filename || 'Frame Details';
            modalUuid.textContent = frame._id || 'N/A';
            modalFilename.textContent = frame.metadata.original_filename || 'Unknown';
            // Parse timestamp for modal - handle both Unix timestamp (C++) and ISO format (Python)
            let modalTimestampText = 'Unknown';
            if (frame.metadata.timestamp) {
                // Try Unix timestamp first (C++ format - string of seconds)
                const timestampNum = parseInt(frame.metadata.timestamp);
                if (!isNaN(timestampNum)) {
                    modalTimestampText = new Date(timestampNum * 1000).toLocaleString();
                } else {
                    // Try ISO format (Python format - ISO string)
                    const timestampDate = new Date(frame.metadata.timestamp);
                    if (!isNaN(timestampDate.getTime())) {
                        modalTimestampText = timestampDate.toLocaleString();
                    }
                }
            }
            modalTimestamp.textContent = modalTimestampText;
            modalSource.textContent = frame.metadata.source || 'Unknown';
            modalAutoSaved.textContent = frame.metadata.auto_saved ? 'Yes' : 'No';
            
            // Add motion detection info if available
            const motionInfo = frame.metadata.motion_detected ? 
                `Motion: ${frame.metadata.motion_detected ? 'Yes' : 'No'}` : '';
            const regionsInfo = frame.metadata.motion_regions ? 
                ` | Regions: ${frame.metadata.motion_regions}` : '';
            const confidenceInfo = frame.metadata.confidence ? 
                ` | Confidence: ${(frame.metadata.confidence * 100).toFixed(1)}%` : '';
            
            // Update modal to show more info
            const additionalInfo = motionInfo + regionsInfo + confidenceInfo;
            if (additionalInfo) {
                modalTitle.textContent += ` (${additionalInfo})`;
            }
            
            imageModal.style.display = 'block';
            document.body.style.overflow = 'hidden';
        } else {
            throw new Error('Frame not found');
        }
    } catch (err) {
        alert('Error loading frame details: ' + err.message);
    }
}

// Close image modal
function closeImageModal() {
    imageModal.style.display = 'none';
    document.body.style.overflow = 'auto';
}

// Copy to clipboard utility
function copyToClipboard(elementId) {
    const element = document.getElementById(elementId);
    const text = element.textContent;
    
    navigator.clipboard.writeText(text).then(function() {
        // Show temporary success message
        const originalText = element.textContent;
        element.textContent = 'Copied!';
        element.style.color = '#48bb78';
        
        setTimeout(() => {
            element.textContent = originalText;
            element.style.color = '';
        }, 2000);
    }).catch(function(err) {
        console.error('Failed to copy text: ', err);
        alert('Failed to copy to clipboard');
    });
}

// Loading and error states
function showLoading() {
    loading.style.display = 'block';
    framesContainer.style.display = 'none';
}

function hideLoading() {
    loading.style.display = 'none';
    framesContainer.style.display = 'grid';
}

function showError(message) {
    errorMessage.textContent = message;
    error.style.display = 'block';
    framesContainer.style.display = 'none';
}

function hideError() {
    error.style.display = 'none';
    framesContainer.style.display = 'grid';
}

// Keyboard shortcuts
document.addEventListener('keydown', function(e) {
    if (e.key === 'Escape') {
        closeImageModal();
    }
    
    if (e.key === 'f' && (e.ctrlKey || e.metaKey)) {
        e.preventDefault();
        searchInput.focus();
    }
    
    if (e.key === 'r' && (e.ctrlKey || e.metaKey)) {
        e.preventDefault();
        refreshBtn.click();
    }
});

// Add placeholder image
const placeholderImage = `
<svg width="300" height="200" xmlns="http://www.w3.org/2000/svg">
    <rect width="100%" height="100%" fill="#f7fafc"/>
    <text x="50%" y="50%" text-anchor="middle" dy=".3em" fill="#a0aec0" font-family="Arial" font-size="16">
        No Image Available
    </text>
</svg>
`;

// Create placeholder image file
const placeholderBlob = new Blob([placeholderImage], { type: 'image/svg+xml' });
const placeholderUrl = URL.createObjectURL(placeholderBlob);

// Update image error handling
document.addEventListener('error', function(e) {
    if (e.target.tagName === 'IMG' && e.target.classList.contains('frame-image')) {
        e.target.src = placeholderUrl;
    }
}, true);
