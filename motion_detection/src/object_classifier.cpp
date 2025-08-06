#include "object_classifier.hpp"        // ObjectClassifier class, ClassificationResult struct
#include "logger.hpp"                   // LOG_INFO, LOG_ERROR, LOG_DEBUG macros
#include <opencv2/imgproc.hpp>          // cv::resize, cv::subtract
#include <fstream>                      // std::ifstream, std::getline
#include <algorithm>                    // std::sort, std::min
#include <sstream>                      // std::stringstream

ObjectClassifier::ObjectClassifier() : modelLoaded(false) {
}

ObjectClassifier::~ObjectClassifier() {
}

bool ObjectClassifier::initialize(const std::string& modelPath, const std::string& labelsPath) {
    try {
        // Load the ONNX model
        net = cv::dnn::readNetFromONNX(modelPath);
        if (net.empty()) {
            LOG_ERROR("Failed to load model from: {}", modelPath);
            return false;
        }
        
        // Load labels
        std::ifstream file(labelsPath);
        if (!file.is_open()) {
            LOG_ERROR("Failed to open labels file: {}", labelsPath);
            return false;
        }
        
        std::string line;
        while (std::getline(file, line)) {
            // Remove carriage return if present
            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }
            labels.push_back(line);
        }
        
        if (labels.empty()) {
            LOG_ERROR("No labels loaded from: {}", labelsPath);
            return false;
        }
        
        modelLoaded = true;
        LOG_INFO("Object classifier initialized successfully. Model: {}, Labels: {}", modelPath, labels.size());
        return true;
        
    } catch (const cv::Exception& e) {
        LOG_ERROR("OpenCV error during model initialization: {}", e.what());
        return false;
    } catch (const std::exception& e) {
        LOG_ERROR("Error during model initialization: {}", e.what());
        return false;
    }
}

ClassificationResult ObjectClassifier::classifyObject(const cv::Mat& croppedImage) {
    if (!modelLoaded || croppedImage.empty()) {
        return ClassificationResult("unknown", 0.0f, -1);
    }
    
    try {
        // Preprocess the image
        cv::Mat preprocessed = preprocessImage(croppedImage);
        
        // Create blob from image
        cv::Mat blob = cv::dnn::blobFromImage(preprocessed, scale, inputSize, mean, false, false);
        
        // Set input and run inference
        net.setInput(blob);
        cv::Mat output = net.forward();
        
        // Process output to get top result
        std::vector<ClassificationResult> results = processOutput(output, 1);
        
        if (!results.empty()) {
            return results[0];
        }
        
    } catch (const cv::Exception& e) {
        LOG_ERROR("OpenCV error during classification: {}", e.what());
    } catch (const std::exception& e) {
        LOG_ERROR("Error during classification: {}", e.what());
    }
    
    return ClassificationResult("unknown", 0.0f, -1);
}

std::vector<ClassificationResult> ObjectClassifier::classifyObjectTopK(const cv::Mat& croppedImage, int k) {
    if (!modelLoaded || croppedImage.empty()) {
        return std::vector<ClassificationResult>();
    }
    
    try {
        // Preprocess the image
        cv::Mat preprocessed = preprocessImage(croppedImage);
        
        // Create blob from image
        cv::Mat blob = cv::dnn::blobFromImage(preprocessed, scale, inputSize, mean, false, false);
        
        // Set input and run inference
        net.setInput(blob);
        cv::Mat output = net.forward();
        
        // Process output to get top k results
        return processOutput(output, k);
        
    } catch (const cv::Exception& e) {
        LOG_ERROR("OpenCV error during classification: {}", e.what());
    } catch (const std::exception& e) {
        LOG_ERROR("Error during classification: {}", e.what());
    }
    
    return std::vector<ClassificationResult>();
}

cv::Mat ObjectClassifier::preprocessImage(const cv::Mat& image) {
    cv::Mat resized;
    cv::resize(image, resized, inputSize);
    
    // Convert to float32 and normalize
    cv::Mat floatImg;
    resized.convertTo(floatImg, CV_32F);
    
    // Subtract mean
    cv::Mat normalized;
    cv::subtract(floatImg, mean, normalized);
    
    return normalized;
}

std::vector<ClassificationResult> ObjectClassifier::processOutput(const cv::Mat& output, int k) {
    std::vector<ClassificationResult> results;
    
    if (output.empty()) {
        return results;
    }
    
    // Get the output data
    cv::Mat scores = output.reshape(1, 1);
    
    // Find top k indices
    std::vector<std::pair<float, int>> scoreIndexPairs;
    for (int i = 0; i < scores.cols; ++i) {
        float score = scores.at<float>(0, i);
        scoreIndexPairs.push_back(std::make_pair(score, i));
    }
    
    // Sort by score (descending)
    std::sort(scoreIndexPairs.begin(), scoreIndexPairs.end(), 
              [](const std::pair<float, int>& a, const std::pair<float, int>& b) {
                  return a.first > b.first;
              });
    
    // Get top k results
    int numResults = std::min(k, static_cast<int>(scoreIndexPairs.size()));
    for (int i = 0; i < numResults; ++i) {
        int classId = scoreIndexPairs[i].second;
        float confidence = scoreIndexPairs[i].first;
        
        std::string label = (classId < static_cast<int>(labels.size())) ? 
                           labels[classId] : "unknown";
        
        results.emplace_back(label, confidence, classId);
    }
    
    return results;
} 