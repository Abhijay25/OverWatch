#include "secret_detector.h"
#include <yaml-cpp/yaml.h>
#include <spdlog/spdlog.h>
#include <sstream>
#include <algorithm>

namespace overwatch {

int SecretDetector::loadPatterns(const std::string& patterns_file) {
    try {
        YAML::Node config = YAML::LoadFile(patterns_file);

        if (!config["patterns"]) {
            spdlog::error("No 'patterns' key found in {}", patterns_file);
            return 0;
        }

        patterns_.clear();

        for (const auto& pattern_node : config["patterns"]) {
            SecretPattern pattern;
            pattern.name = pattern_node["name"].as<std::string>();

            std::string regex_str = pattern_node["regex"].as<std::string>();
            try {
                pattern.regex_pattern = std::regex(regex_str, std::regex::ECMAScript | std::regex::icase);
            } catch (const std::regex_error& e) {
                spdlog::error("Invalid regex for pattern '{}': {}", pattern.name, e.what());
                continue;
            }

            if (pattern_node["files"]) {
                for (const auto& file_pattern : pattern_node["files"]) {
                    pattern.file_patterns.push_back(file_pattern.as<std::string>());
                }
            }

            patterns_.push_back(pattern);
            spdlog::debug("Loaded pattern: {} (files: {})", pattern.name, pattern.file_patterns.size());
        }

        spdlog::info("Loaded {} secret detection patterns from {}", patterns_.size(), patterns_file);
        return patterns_.size();

    } catch (const YAML::Exception& e) {
        spdlog::error("Failed to load patterns from {}: {}", patterns_file, e.what());
        return 0;
    }
}

bool SecretDetector::matchesFilePattern(const std::string& filename, const std::string& pattern) {
    // Simple glob matching
    // "*" matches everything
    if (pattern == "*") {
        return true;
    }

    // "*.ext" matches files ending with .ext
    if (pattern[0] == '*' && pattern.find('*', 1) == std::string::npos) {
        std::string suffix = pattern.substr(1);
        return filename.size() >= suffix.size() &&
               filename.compare(filename.size() - suffix.size(), suffix.size(), suffix) == 0;
    }

    // Exact match
    if (filename == pattern) {
        return true;
    }

    // Check if filename ends with the pattern (for paths)
    if (filename.size() >= pattern.size()) {
        std::string filename_end = filename.substr(filename.size() - pattern.size());
        if (filename_end == pattern) {
            return true;
        }
    }

    return false;
}

std::string SecretDetector::maskMatch(const std::string& match) {
    // Show first 10 and last 4 characters, mask the middle
    if (match.length() <= 20) {
        // For short matches, just show length
        return "[REDACTED:" + std::to_string(match.length()) + " chars]";
    }

    std::string masked = match.substr(0, 10) + "..." + match.substr(match.length() - 4);
    return masked;
}

std::vector<Finding> SecretDetector::scanFile(
    const std::string& content,
    const std::string& filename,
    const std::string& repo_owner,
    const std::string& repo_name,
    const std::string& file_path,
    const std::string& repo_url,
    const std::string& file_url
) {
    std::vector<Finding> findings;

    // Split content into lines for line number tracking
    std::vector<std::string> lines;
    std::stringstream ss(content);
    std::string line;
    while (std::getline(ss, line)) {
        lines.push_back(line);
    }

    // Check each pattern
    for (const auto& pattern : patterns_) {
        // Check if pattern applies to this file
        bool matches_file = false;
        for (const auto& file_pattern : pattern.file_patterns) {
            if (matchesFilePattern(filename, file_pattern)) {
                matches_file = true;
                break;
            }
        }

        if (!matches_file) {
            continue;
        }

        // Search for pattern in each line
        for (size_t line_num = 0; line_num < lines.size(); line_num++) {
            const std::string& line_content = lines[line_num];

            std::smatch match;
            std::string::const_iterator search_start(line_content.cbegin());

            while (std::regex_search(search_start, line_content.cend(), match, pattern.regex_pattern)) {
                Finding finding;
                finding.repo_owner = repo_owner;
                finding.repo_name = repo_name;
                finding.file_path = file_path;
                finding.line_number = line_num + 1;  // 1-indexed
                finding.secret_type = pattern.name;
                finding.matched_text = maskMatch(match.str());
                finding.repo_url = repo_url;
                finding.file_url = file_url;

                findings.push_back(finding);

                spdlog::debug("Found {} in {}:{}", pattern.name, file_path, finding.line_number);

                // Move past this match to find more
                search_start = match.suffix().first;
            }
        }
    }

    return findings;
}

} // namespace overwatch
