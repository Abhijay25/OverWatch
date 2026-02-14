#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace overwatch {

/**
 * Represents a GitHub repository
 */
struct Repository {
    std::string owner;       // Repository owner username
    std::string name;        // Repository name
    std::string url;         // GitHub URL
    int stars;               // Star count
    std::string language;    // Primary language
};

/**
 * GitHub API client for making authenticated requests
 */
class GitHubClient {
public:
    /**
     * Constructor: Create a GitHub client
     * @param token GitHub API token (empty string for unauthenticated)
     */
    explicit GitHubClient(const std::string& token);

    /**
     * Get current rate limit status
     * @return JSON object with rate limit info
     */
    nlohmann::json getRateLimit();

    /**
     * Search for repositories on GitHub
     * @param query Search query (e.g., "language:Python stars:<10")
     * @param max_results Maximum number of repositories to return
     * @return Vector of Repository objects
     */
    std::vector<Repository> searchRepositories(const std::string& query, int max_results = 30);

private:
    std::string token_;      // Member variable: stores the token
    std::string base_url_;   // Member variable: GitHub API base URL
};

} // namespace overwatch
