#include "Config.hpp"

#include <fstream>
#include <iostream>

namespace magda {

Config& Config::getInstance() {
    static Config instance;
    return instance;
}

void Config::saveToFile(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Failed to open config file for writing: " << filename << std::endl;
        return;
    }

    // Simple key-value format for now (later could be JSON)
    file << "defaultTimelineLength=" << defaultTimelineLength << std::endl;
    file << "defaultZoomViewDuration=" << defaultZoomViewDuration << std::endl;
    file << "minZoomLevel=" << minZoomLevel << std::endl;
    file << "maxZoomLevel=" << maxZoomLevel << std::endl;
    file << "zoomInSensitivity=" << zoomInSensitivity << std::endl;
    file << "zoomOutSensitivity=" << zoomOutSensitivity << std::endl;
    file << "zoomInSensitivityShift=" << zoomInSensitivityShift << std::endl;
    file << "zoomOutSensitivityShift=" << zoomOutSensitivityShift << std::endl;

    file.close();
    std::cout << "Config saved to: " << filename << std::endl;
}

void Config::loadFromFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cout << "Config file not found, using defaults: " << filename << std::endl;
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        size_t equalPos = line.find('=');
        if (equalPos == std::string::npos)
            continue;

        std::string key = line.substr(0, equalPos);
        std::string value = line.substr(equalPos + 1);

        try {
            double numValue = std::stod(value);

            if (key == "defaultTimelineLength") {
                defaultTimelineLength = numValue;
            } else if (key == "defaultZoomViewDuration") {
                defaultZoomViewDuration = numValue;
            } else if (key == "minZoomLevel") {
                minZoomLevel = numValue;
            } else if (key == "maxZoomLevel") {
                maxZoomLevel = numValue;
            } else if (key == "zoomInSensitivity") {
                zoomInSensitivity = numValue;
            } else if (key == "zoomOutSensitivity") {
                zoomOutSensitivity = numValue;
            } else if (key == "zoomInSensitivityShift") {
                zoomInSensitivityShift = numValue;
            } else if (key == "zoomOutSensitivityShift") {
                zoomOutSensitivityShift = numValue;
            }
        } catch (const std::exception& e) {
            std::cerr << "Error parsing config value: " << key << "=" << value << std::endl;
        }
    }

    file.close();
    std::cout << "Config loaded from: " << filename << std::endl;
}

}  // namespace magda
