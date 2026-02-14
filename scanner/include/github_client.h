#pragma once

#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>

namespace overwatch {

/**
 * Structure representing a GitHub repository
 */
struct Repository {
    std::string owner;
    std::string name;
    std::string full_name;
    std::string url;
    int stars;
    std::string created_at;
    std::string language;
};

/**
 * Structure representing a file in a repository
 */
struct FileInfo {
    std::string path;
    std::string url;
    std::string download_url;
    std::string sha;
    int size;
};

/**
 * Structure representing API rate limit info
 */
struct RateLimit {
    int limit;
    int remaining;
    int reset_timestamp;

    bool is_exhausted() const { return remaining < 10; }
    int seconds_until_reset(int current_time) const;
};

/**
 * GitHub API client for searching repositories and reading files
 *
 * This client:
 * - Searches for repositories matching criteria
 * - Reads file contents via API (no local cloning)
 * - Handles authentication and rate limiting
 */
class GitHubClient {
public:
    /**
     * Construct a GitHub client
     * @param token GitHub personal access token (optional, but recommended for higher rate limits)
     */
    explicit GitHubClient(const std::string& token = "");

    /**
     * Search for repositories matching query
     * @param query Search query (e.g., "language:Python stars:<10")
     * @param max_results Maximum number of results to return (up to 1000)
     * @return Vector of repositories
     */
    std::vector<Repository> searchRepositories(const std::string& query, int max_results = 30);

    /**
     * Get file contents from a repository
     * @param owner Repository owner
     * @param repo Repository name
     * @param path File path in repository
     * @return File contents as string, or nullopt if not found
     */
    std::optional<std::string> getFileContents(
        const std::string& owner,
        const std::string& repo,
        const std::string& path
    );

    /**
     * Search for files in repositories
     * @param query Search query (e.g., "filename:.env repo:owner/name")
     * @param max_results Maximum number of results
     * @return Vector of file information
     */
    std::vector<FileInfo> searchCode(const std::string& query, int max_results = 30);

    /**
     * Get current rate limit status
     * @return Rate limit information
     */
    RateLimit getRateLimit();

    /**
     * Check if rate limit is exhausted and sleep if needed
     * @return true if we had to sleep, false otherwise
     */
    bool checkAndHandleRateLimit();

private:
    std::string token_;
    std::string base_url_;

    // Helper to make authenticated GET requests
    nlohmann::json makeRequest(const std::string& endpoint, const std::string& accept_header = "application/vnd.github+json");

    // Helper to extract rate limit from response headers
    void updateRateLimitFromHeaders(const std::map<std::string, std::string>& headers);

    // Current rate limit cache
    std::optional<RateLimit> cached_rate_limit_;
};

} // namespace overwatch
