#include <opencv2/opencv.hpp>
#include <pybind11/embed.h>
#include <pybind11/numpy.h>
#include <string>
#include <iostream>

namespace py = pybind11;

// Helper function to convert cv::Mat to numpy array
py::array_t<unsigned char> cv_mat_to_numpy(const cv::Mat& mat) {
    // Ensure the matrix is continuous
    cv::Mat continuous_mat;
    if (!mat.isContinuous()) {
        continuous_mat = mat.clone();
    } else {
        continuous_mat = mat;
    }
    
    // Get the shape and data type
    std::vector<size_t> shape = {static_cast<size_t>(continuous_mat.rows), 
                                 static_cast<size_t>(continuous_mat.cols), 
                                 static_cast<size_t>(continuous_mat.channels())};
    
    // Create numpy array
    py::array_t<unsigned char> numpy_array(shape, continuous_mat.data);
    return numpy_array;
}

std::string save_frame_to_mongodb(const cv::Mat& frame, const std::string& metadata_json) {
    try {
        // Import Python modules
        py::module sys = py::module::import("sys");
        py::module os = py::module::import("os");
        
        // Add the src directory to Python path
        std::string current_dir = os.attr("getcwd")().cast<std::string>();
        sys.attr("path").attr("insert")(0, current_dir + "/src");
        
        // Add virtual environment site-packages to Python path
        std::string venv_site_packages = current_dir + "/venv/lib/python3.13/site-packages";
        sys.attr("path").attr("insert")(0, venv_site_packages);
        
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
}

std::string save_frames_to_mongodb(const cv::Mat& original_frame, const cv::Mat& processed_frame, const std::string& metadata_json) {
    try {
        // Import Python modules
        py::module sys = py::module::import("sys");
        py::module os = py::module::import("os");
        
        // Add the src directory to Python path
        std::string current_dir = os.attr("getcwd")().cast<std::string>();
        sys.attr("path").attr("insert")(0, current_dir + "/src");
        
        // Add virtual environment site-packages to Python path
        std::string venv_site_packages = current_dir + "/venv/lib/python3.13/site-packages";
        sys.attr("path").attr("insert")(0, venv_site_packages);
        
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
}
