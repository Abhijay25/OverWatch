#include <iostream>
#include <cstdlib>
#include <spdlog/spdlog.h>
#include "github_client.h"

int main() {
    spdlog::set_level(spdlog::level::info);

    spdlog::info("OverWatch Scanner v0.1.0");

    // Get GitHub token from environment
    const char* token_env = std::getenv("GITHUB_TOKEN");
    std::string token = token_env ? token_env : "";

    // Create GitHub client
    overwatch::GitHubClient client(token);

    try {
        // Get rate limit
        auto rate_data = client.getRateLimit();

        // Extract values
        int limit = rate_data["rate"]["limit"];
        int remaining = rate_data["rate"]["remaining"];

        spdlog::info("Rate limit: {}/{} requests remaining", remaining, limit);

    } catch (const std::exception& e) {
        spdlog::error("Error: {}", e.what());
        return 1;
    }

    return 0;
}
