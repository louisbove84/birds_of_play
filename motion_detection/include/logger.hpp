#ifndef LOGGER_HPP
#define LOGGER_HPP

#include <spdlog/spdlog.h>
#include <string>

class Logger {
public:
    // Delete copy and assignment operators to enforce singleton pattern
    Logger(const Logger&) = delete;
    void operator=(const Logger&) = delete;

    // Static method to get the single instance of the logger
    static std::shared_ptr<spdlog::logger>& getInstance();

    // Initialize the logger from configuration settings
    static void init(const std::string& logLevel, const std::string& logFile, bool logToFile);

private:
    // Private constructor to prevent direct instantiation
    Logger() {} 

    // The single, static instance of the logger
    static std::shared_ptr<spdlog::logger> loggerInstance;
};

#endif // LOGGER_HPP 