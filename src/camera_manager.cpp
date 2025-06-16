#include "camera_manager.hpp"

CameraManager::CameraManager() : isInitialized_(false) {}

CameraManager::~CameraManager() {
    release();
}

bool CameraManager::initialize(int deviceId) {
    if (isInitialized_) {
        release();
    }
    
    camera_.open(deviceId);
    if (!camera_.isOpened()) {
        return false;
    }
    
    isInitialized_ = true;
    return true;
}

bool CameraManager::getFrame(cv::Mat& frame) {
    if (!isInitialized_) {
        return false;
    }
    
    return camera_.read(frame);
}

void CameraManager::release() {
    if (isInitialized_) {
        camera_.release();
        isInitialized_ = false;
    }
} 