#include "logger.hpp"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/pattern_formatter.h>
#include <memory>

std::shared_ptr<spdlog::logger> Logger::loggerInstance = nullptr;

std::shared_ptr<spdlog::logger>& Logger::getInstance() {
    if (!loggerInstance) {
        throw std::runtime_error("Logger not initialized. Call Logger::init() first.");
    }
    return loggerInstance;
}

void Logger::init(const std::string& logLevel, const std::string& logFile, bool logToFile) {
    std::vector<spdlog::sink_ptr> sinks;

    // Always include colored console output
    auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
    sinks.push_back(consoleSink);

    // Optional file sink
    if (logToFile) {
        auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(logFile, true);
        sinks.push_back(fileSink);
    }

    loggerInstance = std::make_shared<spdlog::logger>("BirdsOfPlayLogger", begin(sinks), end(sinks));
    spdlog::register_logger(loggerInstance);

    // Set logging pattern: [timestamp] [level] [source:line function] message
    loggerInstance->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%s:%# %!] %v");

    // Set log level
    if (logLevel == "debug")
        loggerInstance->set_level(spdlog::level::debug);
    else if (logLevel == "trace")
        loggerInstance->set_level(spdlog::level::trace);
    else if (logLevel == "warn")
        loggerInstance->set_level(spdlog::level::warn);
    else if (logLevel == "error")
        loggerInstance->set_level(spdlog::level::err);
    else if (logLevel == "critical")
        loggerInstance->set_level(spdlog::level::critical);
    else
        loggerInstance->set_level(spdlog::level::info);  // default

    loggerInstance->flush_on(spdlog::level::info);
}
