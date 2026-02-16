#include "github_client.h"
#include "base64.h"
#include <cpr/cpr.h>
#include <spdlog/spdlog.h>

namespace overwatch {

// Constructor 
GitHubClient::GitHubClient(const std::string& token)
    : token_(token), base_url_("https://api.github.com")
{
    if (token_.empty()) {
        spdlog::warn("GitHubClient created without token - using unauthenticated API");
    } else {
        spdlog::debug("GitHubClient created with token");
    }
}

// Validate Github Token
bool GitHubClient::validateToken() {
    if (token_.empty()) {
        return true;  // No token is okay (uses unauthenticated API)
    }

    spdlog::debug("Validating GitHub token");

    // Build headers
    cpr::Header headers = {{"User-Agent", "OverWatch-Scanner"}};
    headers["Authorization"] = "Bearer " + token_;

    // Try to access user endpoint (requires authentication)
    cpr::Response r = cpr::Get(
        cpr::Url{base_url_ + "/user"},
        headers
    );

    if (r.status_code == 401) {
        spdlog::error("GitHub token is invalid or expired!");
        spdlog::error("Please check your token at: https://github.com/settings/tokens");
        return false;
    }

    if (r.status_code == 403) {
        spdlog::error("GitHub token lacks required permissions!");
        spdlog::error("Token needs 'public_repo' scope");
        return false;
    }

    if (r.status_code != 200) {
        spdlog::warn("Could not validate token: HTTP {}", r.status_code);
        return false;
    }

    spdlog::debug("Token is valid");
    return true;
}

// getRateLimit
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

// Search Repos with Given Filters
std::vector<Repository> GitHubClient::searchRepositories(const std::string& query, int max_results) {
    spdlog::info("Searching repositories with query: {}", query);

    std::vector<Repository> repositories;

    // Build headers
    cpr::Header headers = {{"User-Agent", "OverWatch-Scanner"}};
    if (!token_.empty()) {
        headers["Authorization"] = "Bearer " + token_;
    }

    // Handle unlimited mode (max_results = 0)
    if (max_results == 0) {
        spdlog::info("Unlimited mode - fetching all available repositories (up to 1000)");

        int page = 1;
        int per_page = 100;  // GitHub API max per page
        int total_fetched = 0;

        while (true) {
            // Make paginated request
            cpr::Response r = cpr::Get(
                cpr::Url{base_url_ + "/search/repositories"},
                cpr::Parameters{{"q", query}, {"per_page", std::to_string(per_page)}, {"page", std::to_string(page)}},
                headers
            );

            // Check status
            if (r.status_code != 200) {
                spdlog::warn("Search failed with status {} on page {}", r.status_code, page);
                break;
            }

            // Parse JSON response
            nlohmann::json response = nlohmann::json::parse(r.text);

            // Extract total count (for info)
            if (page == 1 && response.contains("total_count")) {
                int total = response["total_count"];
                spdlog::info("Query matches {} total repositories", total);
            }

            // Extract repositories from array
            if (response.contains("items") && !response["items"].empty()) {
                for (const auto& item : response["items"]) {
                    Repository repo;
                    repo.owner = item["owner"]["login"];
                    repo.name = item["name"];
                    repo.url = item["html_url"];
                    repo.stars = item["stargazers_count"];
                    repo.language = item["language"].is_null() ? "" : item["language"];

                    repositories.push_back(repo);
                    total_fetched++;
                }

                spdlog::debug("Fetched page {} - {} repositories so far", page, total_fetched);

                // GitHub limits to 1000 results max, stop if we hit it
                if (total_fetched >= 1000) {
                    spdlog::info("Reached GitHub's 1000 result limit");
                    break;
                }

                page++;
            } else {
                // No more results
                break;
            }
        }

        spdlog::info("Found {} repositories total", repositories.size());
        return repositories;
    }

    // Limited mode - single page fetch
    cpr::Response r = cpr::Get(
        cpr::Url{base_url_ + "/search/repositories"},
        cpr::Parameters{{"q", query}, {"per_page", std::to_string(max_results)}},
        headers
    );

    // Check status
    if (r.status_code != 200) {
        spdlog::error("Search failed with status {}", r.status_code);
        return repositories;
    }

    // Parse JSON response
    nlohmann::json response = nlohmann::json::parse(r.text);

    // Extract repositories from array
    if (response.contains("items")) {
        for (const auto& item : response["items"]) {
            Repository repo;
            repo.owner = item["owner"]["login"];
            repo.name = item["name"];
            repo.url = item["html_url"];
            repo.stars = item["stargazers_count"];
            repo.language = item["language"].is_null() ? "" : item["language"];

            repositories.push_back(repo);
        }
    }

    spdlog::info("Found {} repositories", repositories.size());
    return repositories;
}

// Get File Contents from Repo
std::string GitHubClient::getFileContent(const std::string& owner, const std::string& repo, const std::string& path) {
    spdlog::debug("Fetching file: {}/{}/{}", owner, repo, path);

    // Build headers
    cpr::Header headers = {{"User-Agent", "OverWatch-Scanner"}};
    if (!token_.empty()) {
        headers["Authorization"] = "Bearer " + token_;
    }

    std::string url = base_url_ + "/repos/" + owner + "/" + repo + "/contents/" + path;
    cpr::Response r = cpr::Get(cpr::Url{url}, headers);

    // Check status
    if (r.status_code != 200) {
        if (r.status_code == 404) {
            spdlog::debug("File not found: {}", path);
        } else {
            spdlog::warn("Failed to fetch file: HTTP {}", r.status_code);
        }
        throw std::runtime_error("Failed to fetch file: " + path);
    }

    // Parse JSON response
    nlohmann::json response = nlohmann::json::parse(r.text);

    // Extract and decode base64 content
    if (!response.contains("content")) {
        throw std::runtime_error("No content field in API response");
    }

    std::string base64_content = response["content"];
    std::string decoded = base64_decode(base64_content, true);  // true = remove linebreaks

    spdlog::debug("Successfully fetched {} bytes", decoded.size());
    return decoded;
}

} 
