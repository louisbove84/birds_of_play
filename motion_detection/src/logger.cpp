#include "logger.hpp"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <vector>

// Define the static logger instance
std::shared_ptr<spdlog::logger> Logger::loggerInstance;

void Logger::init(const std::string& logLevel, const std::string& logFile, bool logToFile) {
    std::vector<spdlog::sink_ptr> sinks;
    
    // Console sink
    sinks.push_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
    
    // File sink (if enabled)
    if (logToFile) {
        sinks.push_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFile, true));
    }
    
    loggerInstance = std::make_shared<spdlog::logger>("BirdsOfPlayLogger", begin(sinks), end(sinks));
    
    // Set the logging level from the config
    loggerInstance->set_level(spdlog::level::from_str(logLevel));
    loggerInstance->flush_on(spdlog::level::info); // Auto-flush for important messages
    
    spdlog::register_logger(loggerInstance);
}

std::shared_ptr<spdlog::logger>& Logger::getInstance() {
    if (!loggerInstance) {
        // Fallback to a default console logger if not initialized
        init("info", "birdsofplay.log", false);
    }
    return loggerInstance;
} 