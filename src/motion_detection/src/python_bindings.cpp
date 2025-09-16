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
    
    // Function to save frames directly to MongoDB
    m.def("save_frame_to_mongodb", [](const cv::Mat& frame, const std::string& metadata_json) {
        try {
            // Import Python modules
            py::module sys = py::module::import("sys");
            py::module os = py::module::import("os");
            
            // Add the src directory to Python path
            std::string current_dir = os.attr("getcwd")().cast<std::string>();
            sys.attr("path").attr("insert")(0, current_dir + "/src");
            
            // Import our MongoDB modules
            py::module db_manager_module = py::module::import("mongodb.database_manager");
            py::module frame_db_module = py::module::import("mongodb.frame_database");
            
            // Get the classes
            py::object DatabaseManager = db_manager_module.attr("DatabaseManager");
            py::object FrameDatabase = frame_db_module.attr("FrameDatabase");
            
            // Create database manager and connect
            py::object db_manager = DatabaseManager();
            db_manager.attr("connect")();
            
            // Create frame database
            py::object frame_db = FrameDatabase(db_manager);
            
            // Convert metadata JSON to Python dict
            py::module json = py::module::import("json");
            py::object metadata = json.attr("loads")(metadata_json);
            
            // Convert cv::Mat to numpy array for Python
            py::array_t<unsigned char> numpy_frame = cv_mat_to_numpy(frame);
            
            // Save frame
            py::object result = frame_db.attr("save_frame")(numpy_frame, metadata);
            
            // Disconnect
            db_manager.attr("disconnect")();
            
            return result.cast<std::string>();
            
        } catch (const py::error_already_set& e) {
            std::cerr << "Python error: " << e.what() << std::endl;
            return std::string("");
        } catch (const std::exception& e) {
            std::cerr << "C++ error: " << e.what() << std::endl;
            return std::string("");
        }
    }, "Save a frame directly to MongoDB");
    
    // Function to save both original and processed frames to MongoDB
    m.def("save_frames_to_mongodb", [](const cv::Mat& original_frame, const cv::Mat& processed_frame, const std::string& metadata_json) {
        try {
            // Import Python modules
            py::module sys = py::module::import("sys");
            py::module os = py::module::import("os");
            
            // Add the src directory to Python path
            std::string current_dir = os.attr("getcwd")().cast<std::string>();
            sys.attr("path").attr("insert")(0, current_dir + "/src");
            
            // Import our MongoDB modules
            py::module db_manager_module = py::module::import("mongodb.database_manager");
            py::module frame_db_module = py::module::import("mongodb.frame_database");
            
            // Get the classes
            py::object DatabaseManager = db_manager_module.attr("DatabaseManager");
            py::object FrameDatabase = frame_db_module.attr("FrameDatabase");
            
            // Create database manager and connect
            py::object db_manager = DatabaseManager();
            db_manager.attr("connect")();
            
            // Create frame database
            py::object frame_db = FrameDatabase(db_manager);
            
            // Convert metadata JSON to Python dict
            py::module json = py::module::import("json");
            py::object metadata = json.attr("loads")(metadata_json);
            
            // Convert cv::Mat to numpy arrays for Python
            py::array_t<unsigned char> numpy_original = cv_mat_to_numpy(original_frame);
            py::array_t<unsigned char> numpy_processed = cv_mat_to_numpy(processed_frame);
            
            // Save both frames
            py::object result = frame_db.attr("save_frame_with_original")(numpy_original, numpy_processed, metadata);
            
            // Disconnect
            db_manager.attr("disconnect")();
            
            return result.cast<std::string>();
            
        } catch (const py::error_already_set& e) {
            std::cerr << "Python error: " << e.what() << std::endl;
            return std::string("");
        } catch (const std::exception& e) {
            std::cerr << "C++ error: " << e.what() << std::endl;
            return std::string("");
        }
    }, "Save both original and processed frames to MongoDB");
}
