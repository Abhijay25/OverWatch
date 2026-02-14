#include "github_client.h"
#include <cpr/cpr.h>
#include <spdlog/spdlog.h>

namespace overwatch {

// Constructor implementation
GitHubClient::GitHubClient(const std::string& token)
    : token_(token), base_url_("https://api.github.com")
{
    // Constructor body (can be empty if initialization list does everything)
    if (token_.empty()) {
        spdlog::warn("GitHubClient created without token - using unauthenticated API");
    } else {
        spdlog::debug("GitHubClient created with token");
    }
}

// getRateLimit implementation
nlohmann::json GitHubClient::getRateLimit() {
    spdlog::info("Fetching rate limit from GitHub API");

    // Build headers
    cpr::Header headers = {{"User-Agent", "OverWatch-Scanner"}};
    if (!token_.empty()) {
        headers["Authorization"] = "Bearer " + token_;
    }

    // Make request
    cpr::Response r = cpr::Get(
        cpr::Url{base_url_ + "/rate_limit"},
        headers
    );

    // Check status
    if (r.status_code != 200) {
        spdlog::error("GitHub API returned status {}", r.status_code);
        throw std::runtime_error("Failed to get rate limit");
    }

    // Parse and return JSON
    return nlohmann::json::parse(r.text);
}

} // namespace overwatch
