#include <iostream>
#include <cstdlib>
#include <spdlog/spdlog.h>
#include "github_client.h"

int main() {
    spdlog::set_level(spdlog::level::info);

    spdlog::info("OverWatch Scanner v0.2.0");

    // Get GitHub token from environment
    const char* token_env = std::getenv("GITHUB_TOKEN");
    std::string token = token_env ? token_env : "";

    // Create GitHub client
    overwatch::GitHubClient client(token);

    try {
        // Test 1: Get rate limit
        spdlog::info("\n=== Test 1: Rate Limit ===");
        auto rate_data = client.getRateLimit();
        int limit = rate_data["rate"]["limit"];
        int remaining = rate_data["rate"]["remaining"];
        spdlog::info("Rate limit: {}/{} requests remaining\n", remaining, limit);

        // Test 2: Search repositories
        spdlog::info("=== Test 2: Search Repositories ===");
        std::string query = "language:Python stars:<5 created:>2026-02-10";
        spdlog::info("Searching for: {}", query);

        auto repos = client.searchRepositories(query, 5);

        spdlog::info("\nResults:");
        for (const auto& repo : repos) {
            spdlog::info("  • {}/{}", repo.owner, repo.name);
            spdlog::info("    Stars: {} | Language: {} | URL: {}",
                        repo.stars,
                        repo.language.empty() ? "N/A" : repo.language,
                        repo.url);
        }

    } catch (const std::exception& e) {
        spdlog::error("Error: {}", e.what());
        return 1;
    }

    return 0;
}
