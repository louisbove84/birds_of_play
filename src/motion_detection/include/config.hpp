#ifndef CONFIG_HPP
#define CONFIG_HPP

#include <yaml-cpp/yaml.h>
#include <string>

class Config {
public:
    static Config& getInstance();
    void load(const std::string& config_path);

    std::string getLogLevel() const;
    std::string getLogFile() const;
    bool getConsoleOut() const;
    std::string getMongoURI() const;
    std::string getDBName() const;
    std::string getCollectionName() const;

private:
    Config() = default;
    Config(const Config&) = delete;
    Config& operator=(const Config&) = delete;

    static Config* instance;
    YAML::Node config_node;
};

#endif // CONFIG_HPP 