#include "config.hpp"
#include "logger.h" // Use the new logger macros
#include <iostream>

Config* Config::instance = nullptr;

Config& Config::getInstance() {
    if (instance == nullptr) {
        instance = new Config();
    }
    return *instance;
}

void Config::load(const std::string& config_path) {
    try {
        config_node = YAML::LoadFile(config_path);
        LOG_INFO("Configuration loaded from {}", config_path);
    } catch (const YAML::Exception& e) {
        LOG_CRITICAL("Failed to load or parse config file: {}. Error: {}", config_path, e.what());
        // For now, we'll proceed with an empty config node, which will cause downstream errors.
    }
}

// ... existing code ... 