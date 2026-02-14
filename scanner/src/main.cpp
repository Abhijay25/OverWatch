#include <iostream>
#include <cstdlib>
#include <cpr/cpr.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

int main() {
    // Get GitHub token from environment variable
    const char* token_env = std::getenv("GITHUB_TOKEN");
    std::string token = token_env ? token_env : "";

    if (token.empty()) {
        spdlog::warn("No GITHUB_TOKEN set - using unauthenticated API");
    } else {
        spdlog::info("Token loaded: {}...", token.substr(0, 8));
    }

    // Make HTTP GET request to GitHub API
    spdlog::info("Requesting rate limit from GitHub API...");

    cpr::Response r = cpr::Get(
        cpr::Url{"https://api.github.com/rate_limit"},
        cpr::Header{
            {"User-Agent", "OverWatch-Scanner"},
            {"Authorization", "Bearer " + token}
        }
    );

    // Check if request succeeded
    if (r.status_code != 200) {
        spdlog::error("Request failed with status: {}", r.status_code);
        return 1;
    }

    // Parse JSON response
    nlohmann::json data = nlohmann::json::parse(r.text);

    // Extract rate limit info
    int limit = data["rate"]["limit"];
    int remaining = data["rate"]["remaining"];

    spdlog::info("Rate limit: {}/{} requests remaining", remaining, limit);

    return 0;
}
