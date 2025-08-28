#ifndef LOGGER_HPP
#define LOGGER_HPP

#define LOG_TRACE(...)    SPDLOG_LOGGER_TRACE(Logger::getInstance(), __VA_ARGS__)
#define LOG_DEBUG(...)    SPDLOG_LOGGER_DEBUG(Logger::getInstance(), __VA_ARGS__)
#define LOG_INFO(...)     SPDLOG_LOGGER_INFO(Logger::getInstance(), __VA_ARGS__)
#define LOG_WARN(...)     SPDLOG_LOGGER_WARN(Logger::getInstance(), __VA_ARGS__)
#define LOG_ERROR(...)    SPDLOG_LOGGER_ERROR(Logger::getInstance(), __VA_ARGS__)
#define LOG_CRITICAL(...) SPDLOG_LOGGER_CRITICAL(Logger::getInstance(), __VA_ARGS__)

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