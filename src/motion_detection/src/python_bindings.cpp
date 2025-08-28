#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
#include <opencv2/opencv.hpp>
#include "motion_processor.hpp"
#include "motion_visualization.hpp"
#include "motion_region_consolidator.hpp"
#include "motion_pipeline.hpp"
#include "logger.hpp"

namespace py = pybind11;

// Convert numpy array to cv::Mat
cv::Mat numpy_to_cv_mat(py::array_t<unsigned char>& input) {
    py::buffer_info buf = input.request();
    
    if (buf.ndim != 3) {
        throw std::runtime_error("Number of dimensions must be 3 (height, width, channels)");
    }
    
    int height = static_cast<int>(buf.shape[0]);
    int width = static_cast<int>(buf.shape[1]);
    int channels = static_cast<int>(buf.shape[2]);
    
    cv::Mat mat(height, width, CV_8UC(channels), buf.ptr);
    return mat.clone(); // Return a copy to ensure data ownership
}

// Convert cv::Mat to numpy array
py::array_t<unsigned char> cv_mat_to_numpy(const cv::Mat& mat) {
    py::array_t<unsigned char> array({mat.rows, mat.cols, mat.channels()});
    py::buffer_info buf = array.request();
    
    unsigned char* ptr = static_cast<unsigned char*>(buf.ptr);
    std::memcpy(ptr, mat.data, mat.total() * mat.elemSize());
    
    return array;
}

// Wrapper class for MotionProcessor
class MotionProcessorWrapper {
private:
    std::unique_ptr<MotionProcessor> processor;
    
public:
    MotionProcessorWrapper(const std::string& config_path = "") {
        // Initialize logger first
        Logger::init("info", "python_bindings.log", false);
        
        // MotionProcessor requires a config path, so we'll use a default one if empty
        std::string actual_config = config_path.empty() ? "config.yaml" : config_path;
        processor = std::make_unique<MotionProcessor>(actual_config);
    }
    
    py::array_t<unsigned char> process_frame(py::array_t<unsigned char>& input_frame) {
        cv::Mat frame = numpy_to_cv_mat(input_frame);
        auto result = processor->processFrame(frame);
        return cv_mat_to_numpy(result.processedFrame);
    }
    
    std::vector<py::dict> get_detections() {
        // For now, return empty list since we need to store the last result
        // This can be enhanced later to store the last ProcessingResult
        return std::vector<py::dict>();
    }
    
    void reset() {
        // Reset by creating a new processor instance
        std::string config_path = "config.yaml"; // Default config
        processor = std::make_unique<MotionProcessor>(config_path);
    }
    
    // Get the last processing result
    py::dict get_last_result() {
        // This would need to be implemented by storing the last ProcessingResult
        // For now, return empty dict
        return py::dict();
    }
};

PYBIND11_MODULE(birds_of_play_python, m) {
    m.doc() = "Python bindings for Birds of Play motion detection library";
    
    // MotionProcessor wrapper
    py::class_<MotionProcessorWrapper>(m, "MotionProcessor")
        .def(py::init<const std::string&>(), py::arg("config_path") = "")
        .def("process_frame", &MotionProcessorWrapper::process_frame, "Process a frame and return the result")
        .def("get_detections", &MotionProcessorWrapper::get_detections, "Get current detections")
        .def("reset", &MotionProcessorWrapper::reset, "Reset the processor state")
        .def("get_last_result", &MotionProcessorWrapper::get_last_result, "Get the last processing result");
}
