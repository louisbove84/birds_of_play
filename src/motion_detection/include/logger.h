#ifndef LOGGER_H
#define LOGGER_H

#include "spdlog/spdlog.h"
#include <memory>
#include <string>

class Logger {
public:
    // The init function now sets up a global default logger
    static void init(const std::string& log_level = "info", const std::string& log_file = "", bool console_out = true);
};

// Define macros to wrap the spdlog calls.
// This is the key to capturing the correct source location.
#define LOG_INFO(...)    spdlog::info(__VA_ARGS__)
#define LOG_WARN(...)    spdlog::warn(__VA_ARGS__)
#define LOG_ERROR(...)   spdlog::error(__VA_ARGS__)
#define LOG_DEBUG(...)   spdlog::debug(__VA_ARGS__)
#define LOG_CRITICAL(...) spdlog::critical(__VA_ARGS__)

#endif // LOGGER_H 