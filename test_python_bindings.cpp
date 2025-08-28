#include <pybind11/embed.h>
#include <pybind11/numpy.h>
#include <iostream>
#include <string>

namespace py = pybind11;

int main() {
    try {
        // Initialize Python interpreter
        py::scoped_interpreter guard{};
        
        std::cout << "Python interpreter initialized successfully!" << std::endl;
        
        // Test basic Python functionality
        py::module sys = py::module::import("sys");
        std::cout << "Python sys module imported successfully!" << std::endl;
        
        // Test if we can import our MongoDB modules
        try {
            py::module os = py::module::import("os");
            std::string current_dir = os.attr("getcwd")().cast<std::string>();
            std::cout << "Current directory: " << current_dir << std::endl;
            
            // Add the src directory to Python path
            sys.attr("path").attr("insert")(0, current_dir + "/src");
            std::cout << "Added src directory to Python path" << std::endl;
            
            // Add virtual environment site-packages to Python path
            std::string venv_site_packages = current_dir + "/venv/lib/python3.13/site-packages";
            sys.attr("path").attr("insert")(0, venv_site_packages);
            std::cout << "Added virtual environment site-packages to Python path" << std::endl;
            
            // Try to import our MongoDB modules
            py::module db_manager_module = py::module::import("mongodb.database_manager");
            std::cout << "MongoDB database_manager module imported successfully!" << std::endl;
            
            py::module frame_db_module = py::module::import("mongodb.frame_database");
            std::cout << "MongoDB frame_database module imported successfully!" << std::endl;
            
            // Test creating objects
            py::object DatabaseManager = db_manager_module.attr("DatabaseManager");
            py::object db_manager = DatabaseManager();
            std::cout << "DatabaseManager object created successfully!" << std::endl;
            
            py::object FrameDatabase = frame_db_module.attr("FrameDatabase");
            py::object frame_db = FrameDatabase(db_manager);
            std::cout << "FrameDatabase object created successfully!" << std::endl;
            
            std::cout << "All Python bindings tests passed!" << std::endl;
            
        } catch (const py::error_already_set& e) {
            std::cerr << "Python error: " << e.what() << std::endl;
            return 1;
        }
        
    } catch (const std::exception& e) {
        std::cerr << "C++ error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
