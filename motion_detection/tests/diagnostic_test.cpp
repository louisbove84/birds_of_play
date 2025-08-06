#include "motion_tracker.hpp"
#include "logger.hpp"
#include <opencv2/opencv.hpp>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <algorithm>

void initLogger() {
    Logger::init("debug", "diagnostic_log.txt", true);
}

void createTestDirectory(const std::string& testName) {
    std::filesystem::create_directories("test_results/" + testName);
}

int main() {
    initLogger();
    createTestDirectory("diagnostic");
    
    std::cout << "🔍 Running diagnostic analysis..." << std::endl;
    
    // Load images
    cv::Mat frame1 = cv::imread("test_image.jpg");
    cv::Mat frame2 = cv::imread("test_image2.jpg");
    
    if (frame1.empty() || frame2.empty()) {
        std::cout << "❌ Failed to load test images" << std::endl;
        return 1;
    }
    
    std::cout << "✅ Loaded images successfully" << std::endl;
    std::cout << "Frame1 size: " << frame1.size() << ", channels: " << frame1.channels() << std::endl;
    std::cout << "Frame2 size: " << frame2.size() << ", channels: " << frame2.channels() << std::endl;
    
    // Convert to grayscale
    cv::Mat gray1, gray2;
    cv::cvtColor(frame1, gray1, cv::COLOR_BGR2GRAY);
    cv::cvtColor(frame2, gray2, cv::COLOR_BGR2GRAY);
    
    // Calculate absolute difference
    cv::Mat diff;
    cv::absdiff(gray1, gray2, diff);
    
    // Get statistics
    cv::Scalar meanVal = cv::mean(diff);
    double minVal, maxVal;
    cv::minMaxLoc(diff, &minVal, &maxVal);
    
    // Calculate mode (most frequent value)
    std::vector<int> histogram(256, 0);
    for (int i = 0; i < diff.rows; i++) {
        for (int j = 0; j < diff.cols; j++) {
            histogram[diff.at<uchar>(i, j)]++;
        }
    }
    
    int mode = 0;
    int maxCount = histogram[0];
    for (int i = 1; i < 256; i++) {
        if (histogram[i] > maxCount) {
            maxCount = histogram[i];
            mode = i;
        }
    }
    
    // Calculate standard deviation
    cv::Scalar meanScalar, stdScalar;
    cv::meanStdDev(diff, meanScalar, stdScalar);
    double stdDev = stdScalar[0];
    
    std::cout << "\n📊 Frame Difference Statistics:" << std::endl;
    std::cout << "Mean difference: " << meanVal[0] << std::endl;
    std::cout << "Mode (most frequent): " << mode << std::endl;
    std::cout << "Standard deviation: " << stdDev << std::endl;
    std::cout << "Min difference: " << minVal << std::endl;
    std::cout << "Max difference: " << maxVal << std::endl;
    
    // Calculate recommended thresholds based on statistics
    double mean = meanVal[0];
    std::vector<int> thresholds;
    
    // Add mode-based thresholds
    thresholds.push_back(mode + 5);
    thresholds.push_back(mode + 10);
    thresholds.push_back(mode + 15);
    
    // Add mean-based thresholds
    thresholds.push_back((int)(mean + stdDev));
    thresholds.push_back((int)(mean + 2 * stdDev));
    thresholds.push_back((int)(mean + 3 * stdDev));
    
    // Add some fixed values for comparison
    thresholds.push_back(20);
    thresholds.push_back(50);
    thresholds.push_back(100);
    
    // Remove duplicates and sort
    std::sort(thresholds.begin(), thresholds.end());
    thresholds.erase(std::unique(thresholds.begin(), thresholds.end()), thresholds.end());
    
    std::cout << "\n🎯 Recommended Thresholds:" << std::endl;
    std::cout << "Mode + 5: " << (mode + 5) << std::endl;
    std::cout << "Mode + 10: " << (mode + 10) << std::endl;
    std::cout << "Mean + 1σ: " << (int)(mean + stdDev) << std::endl;
    std::cout << "Mean + 2σ: " << (int)(mean + 2 * stdDev) << std::endl;
    std::cout << "Mean + 3σ: " << (int)(mean + 3 * stdDev) << std::endl;
    
    std::cout << "\n🎯 Threshold Analysis:" << std::endl;
    for (int thresh : thresholds) {
        cv::Mat binary;
        cv::threshold(diff, binary, thresh, 255, cv::THRESH_BINARY);
        
        int totalPixels = binary.rows * binary.cols;
        int whitePixels = cv::countNonZero(binary);
        double percentage = (double)whitePixels / totalPixels * 100.0;
        
        std::cout << "Threshold " << thresh << ": " << percentage << "% motion" << std::endl;
        
        // Save some key thresholds
        if (thresh == 30 || thresh == 50 || thresh == 75) {
            cv::imwrite("test_results/diagnostic/threshold_" + std::to_string(thresh) + ".jpg", binary);
        }
    }
    
    // Save diagnostic images
    cv::imwrite("test_results/diagnostic/frame1_gray.jpg", gray1);
    cv::imwrite("test_results/diagnostic/frame2_gray.jpg", gray2);
    cv::imwrite("test_results/diagnostic/raw_difference.jpg", diff);
    
    // Create a histogram of difference values
    std::vector<int> hist(256, 0);
    for (int i = 0; i < diff.rows; i++) {
        for (int j = 0; j < diff.cols; j++) {
            hist[diff.at<uchar>(i, j)]++;
        }
    }
    
    std::cout << "\n📈 Difference Value Distribution (first 20 values):" << std::endl;
    for (int i = 0; i < 20; i++) {
        if (hist[i] > 0) {
            std::cout << "Value " << i << ": " << hist[i] << " pixels" << std::endl;
        }
    }
    
    // Find optimal threshold (one that gives reasonable motion percentage)
    int optimalThreshold = mode + 10;  // Start with mode + 10 as default
    for (int thresh : thresholds) {
        cv::Mat binary;
        cv::threshold(diff, binary, thresh, 255, cv::THRESH_BINARY);
        int totalPixels = binary.rows * binary.cols;
        int whitePixels = cv::countNonZero(binary);
        double percentage = (double)whitePixels / totalPixels * 100.0;
        
        // Look for threshold that gives 0.1% to 2% motion (reasonable range)
        if (percentage >= 0.1 && percentage <= 2.0) {
            optimalThreshold = thresh;
            break;
        }
    }
    
    std::cout << "\n✨ Diagnostic complete! Check test_results/diagnostic/ folder" << std::endl;
    std::cout << "\n🎯 Threshold Recommendations:" << std::endl;
    std::cout << "📊 Statistical optimal: " << optimalThreshold << " (gives reasonable motion %)" << std::endl;
    std::cout << "🔧 Conservative (less noise): " << (int)(mean + 2 * stdDev) << std::endl;
    std::cout << "⚡ Sensitive (more detection): " << (mode + 5) << std::endl;
    std::cout << "\n💡 Start with: " << optimalThreshold << " and adjust based on your needs!" << std::endl;
    
    return 0;
}
