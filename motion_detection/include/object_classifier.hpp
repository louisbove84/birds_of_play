#ifndef OBJECT_CLASSIFIER_HPP
#define OBJECT_CLASSIFIER_HPP

#include <opencv2/core.hpp>             // cv::Mat, cv::Size, cv::Scalar
#include <opencv2/dnn.hpp>              // cv::dnn::Net, cv::dnn::readNetFromONNX
#include <string>                       // std::string
#include <vector>                       // std::vector

struct ClassificationResult {
    std::string label;
    float confidence;
    int class_id;
    
    ClassificationResult() : confidence(0.0f), class_id(-1) {}
    ClassificationResult(const std::string& lbl, float conf, int id) 
        : label(lbl), confidence(conf), class_id(id) {}
};

class ObjectClassifier {
public:
    ObjectClassifier();
    ~ObjectClassifier();
    
    bool initialize(const std::string& modelPath, const std::string& labelsPath);
    ClassificationResult classifyObject(const cv::Mat& croppedImage);
    std::vector<ClassificationResult> classifyObjectTopK(const cv::Mat& croppedImage, int k = 5);
    
    // Getter for checking if model is loaded
    bool isModelLoaded() const { return modelLoaded; }
    
private:
    cv::dnn::Net net;
    std::vector<std::string> labels;
    bool modelLoaded;
    
    // Preprocessing parameters for SqueezeNet
    const cv::Size inputSize = cv::Size(227, 227);
    const cv::Scalar mean = cv::Scalar(104.0, 117.0, 123.0); // BGR mean values
    const float scale = 1.0f;
    
    cv::Mat preprocessImage(const cv::Mat& image);
    std::vector<ClassificationResult> processOutput(const cv::Mat& output, int k = 5);
};

#endif // OBJECT_CLASSIFIER_HPP 