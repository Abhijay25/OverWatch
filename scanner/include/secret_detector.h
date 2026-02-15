#pragma once

#include <string>
#include <vector>
#include <regex>

namespace overwatch {

struct Pattern {
    std::string name;
    std::regex regex;
    std::vector<std::string> files;
};

struct Match {
    std::string pattern_name;
    int line_number;
    std::string matched_text;
};

class SecretDetector {
public:
    SecretDetector();

    /**
     * Load patterns from YAML file
     * @param yaml_path Path to patterns.yaml
     */
    void loadPatterns(const std::string& yaml_path);

    /**
     * Scan content for secrets
     * @param content File content to scan
     * @param filename Name of file being scanned (for file-specific patterns)
     * @return Vector of matches found
     */
    std::vector<Match> scanContent(const std::string& content, const std::string& filename);

private:
    std::vector<Pattern> patterns_;

    bool fileMatchesPattern(const std::string& filename, const std::vector<std::string>& file_patterns);
};

} // namespace overwatch
