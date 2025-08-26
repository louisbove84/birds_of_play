#include "motion_processor.hpp"
#include "logger.hpp"
#include <iostream>

int main() {
    // Initialize logger
    Logger::init("info", "test.log", false);
    
    // Test with clean config
    MotionProcessor processor("config_clean.yaml");
    
    std::cout << "Config values loaded:" << std::endl;
    std::cout << "Min contour area: " << processor.getMinContourArea() << std::endl;
    
    return 0;
}
