#pragma once

#include <string>
#include <vector>
#include <regex>
#include <memory>

namespace overwatch {

/**
 * Pattern for detecting secrets
 */
struct SecretPattern {
    std::string name;           // e.g., "GitHub Token"
    std::regex regex_pattern;   // Compiled regex
    std::vector<std::string> file_patterns;  // File glob patterns (e.g., "*.env", "*")
};

/**
 * A detected secret in a file
 */
struct Finding {
    std::string repo_owner;
    std::string repo_name;
    std::string file_path;
    int line_number;
    std::string secret_type;    // Pattern name
    std::string matched_text;   // Truncated/masked match
    std::string repo_url;
    std::string file_url;
};

/**
 * Secret detector that scans file contents for patterns
 */
class SecretDetector {
public:
    /**
     * Load patterns from YAML config file
     * @param patterns_file Path to patterns.yaml
     * @return Number of patterns loaded
     */
    int loadPatterns(const std::string& patterns_file);

    /**
     * Scan file contents for secrets
     * @param content File contents
     * @param filename File name (for pattern matching)
     * @param repo_owner Repository owner
     * @param repo_name Repository name
     * @param file_path Full path in repo
     * @param repo_url Repository URL
     * @param file_url File URL
     * @return Vector of findings
     */
    std::vector<Finding> scanFile(
        const std::string& content,
        const std::string& filename,
        const std::string& repo_owner,
        const std::string& repo_name,
        const std::string& file_path,
        const std::string& repo_url,
        const std::string& file_url
    );

    /**
     * Get number of loaded patterns
     */
    size_t getPatternCount() const { return patterns_.size(); }

private:
    std::vector<SecretPattern> patterns_;

    // Check if filename matches file pattern (simple glob matching)
    bool matchesFilePattern(const std::string& filename, const std::string& pattern);

    // Mask/truncate sensitive data in match
    std::string maskMatch(const std::string& match);
};

} // namespace overwatch
