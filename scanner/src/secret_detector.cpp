#include "secret_detector.h"
#include <yaml-cpp/yaml.h>
#include <spdlog/spdlog.h>
#include <sstream>
#include <algorithm>

namespace overwatch {

SecretDetector::SecretDetector() {
}

void SecretDetector::loadPatterns(const std::string& yaml_path) {
    spdlog::info("Loading patterns from: {}", yaml_path);

    YAML::Node config = YAML::LoadFile(yaml_path);

    if (!config["patterns"]) {
        throw std::runtime_error("No 'patterns' key found in YAML");
    }

    for (const auto& pattern_node : config["patterns"]) {
        Pattern pattern;
        pattern.name = pattern_node["name"].as<std::string>();

        std::string regex_str = pattern_node["regex"].as<std::string>();
        pattern.regex = std::regex(regex_str, std::regex::icase);

        if (pattern_node["files"]) {
            for (const auto& file : pattern_node["files"]) {
                pattern.files.push_back(file.as<std::string>());
            }
        }

        patterns_.push_back(pattern);
    }

    spdlog::info("Loaded {} patterns", patterns_.size());
}

std::vector<Match> SecretDetector::scanContent(const std::string& content, const std::string& filename) {
    std::vector<Match> matches;

    std::istringstream stream(content);
    std::string line;
    int line_number = 0;

    while (std::getline(stream, line)) {
        line_number++;

        for (const auto& pattern : patterns_) {
            // Check if this pattern applies to this file type
            if (!fileMatchesPattern(filename, pattern.files)) {
                continue;
            }

            // Search for pattern in this line
            std::smatch match;
            if (std::regex_search(line, match, pattern.regex)) {
                Match found;
                found.pattern_name = pattern.name;
                found.line_number = line_number;
                found.matched_text = match[0].str();
                matches.push_back(found);
            }
        }
    }

    return matches;
}

bool SecretDetector::fileMatchesPattern(const std::string& filename,
                                        const std::vector<std::string>& file_patterns) {
    // If pattern applies to all files
    if (std::find(file_patterns.begin(), file_patterns.end(), "*") != file_patterns.end()) {
        return true;
    }

    // Check if filename matches any of the patterns
    for (const auto& pattern : file_patterns) {
        // Simple wildcard matching: *.ext
        if (pattern[0] == '*' && pattern.size() > 1) {
            std::string extension = pattern.substr(1);
            if (filename.size() >= extension.size() &&
                filename.substr(filename.size() - extension.size()) == extension) {
                return true;
            }
        }
        // Exact filename match
        else if (filename == pattern) {
            return true;
        }
    }

    return false;
}

} 