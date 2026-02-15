#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace overwatch {

/**
 * Represents a GitHub repository
 */
struct Repository {
    std::string owner;
    std::string name;
    std::string url;
    int stars;
    std::string language;
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
     * Validate that the token is working
     * @return true if token is valid, false otherwise
     */
    bool validateToken();

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

    /**
     * Get file contents from a repository
     * @param owner Repository owner
     * @param repo Repository name
     * @param path File path within repository
     * @return Decoded file contents as string
     */
    std::string getFileContent(const std::string& owner, const std::string& repo, const std::string& path);

private:
    std::string token_;
    std::string base_url_;
};

} // namespace overwatch
