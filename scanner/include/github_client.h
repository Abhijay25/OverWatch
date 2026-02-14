#pragma once

#include <string>
#include <nlohmann/json.hpp>

namespace overwatch {

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

private:
    std::string token_;      // Member variable: stores the token
    std::string base_url_;   // Member variable: GitHub API base URL
};

} // namespace overwatch
