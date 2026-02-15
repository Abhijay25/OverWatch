#include <iostream>
#include <cstdlib>
#include <spdlog/spdlog.h>
#include "github_client.h"
#include "secret_detector.h"
#include "scanner.h"

int main() {
    spdlog::set_level(spdlog::level::info);

    spdlog::info("OverWatch Scanner v0.3.0");

    // Get GitHub token from environment
    const char* token_env = std::getenv("GITHUB_TOKEN");
    std::string token = token_env ? token_env : "";

    if (token.empty()) {
        spdlog::warn("No GITHUB_TOKEN found - using unauthenticated API (lower rate limits)");
    }

    try {
        // Create GitHub client
        overwatch::GitHubClient client(token);

        // Validate token if provided
        if (!token.empty() && !client.validateToken()) {
            spdlog::error("Failed to validate GitHub token. Please check");
            spdlog::error("  1. Token is not expired: https://github.com/settings/tokens");
            spdlog::error("  2. Token has 'public_repo' scope");
            spdlog::error("  3. Token format is correct (starts with ghp_)");
            return 1;
        }

        // Check rate limit
        auto rate_data = client.getRateLimit();
        int remaining = rate_data["rate"]["remaining"];
        int limit = rate_data["rate"]["limit"];
        spdlog::info("API rate limit: {}/{} requests remaining", remaining, limit);

        if (!token.empty() && limit == 60) {
            spdlog::warn("Token might not be working - using unauthenticated rate limit");
            spdlog::warn("Authenticated tokens should have 5000 requests/hour");
        }

        // Create and configure secret detector
        overwatch::SecretDetector detector;
        detector.loadPatterns("../config/patterns.yaml");

        // Create scanner
        overwatch::Scanner scanner(client, detector, "../data/findings.jsonl");

        // Run scan
        std::string query = "language:Python stars:<10 created:>2026-02-10";
        int max_repos = 5;

        spdlog::info("Search query: {}", query);
        scanner.run(query, max_repos);

        spdlog::info("Scan complete! Check ../data/findings.jsonl for results");

    } catch (const std::exception& e) {
        spdlog::error("Error: {}", e.what());
        return 1;
    }

    return 0;
}
