#include "github_client.h"
#include <cpr/cpr.h>
#include <spdlog/spdlog.h>
#include <sstream>
#include <iomanip>

namespace overwatch {

// Helper function to URL-encode a string
std::string url_encode(const std::string& value) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (char c : value) {
        // Keep alphanumeric and safe characters
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
        } else {
            // Percent-encode everything else
            escaped << std::uppercase;
            escaped << '%' << std::setw(2) << int((unsigned char)c);
            escaped << std::nouppercase;
        }
    }

    return escaped.str();
}

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

// searchRepositories implementation
std::vector<Repository> GitHubClient::searchRepositories(const std::string& query, int max_results) {
    spdlog::info("Searching repositories with query: {}", query);

    std::vector<Repository> repositories;  // Create empty vector

    // Build headers
    cpr::Header headers = {{"User-Agent", "OverWatch-Scanner"}};
    if (!token_.empty()) {
        headers["Authorization"] = "Bearer " + token_;
    }

    // Build API URL with query parameters (encode the query!)
    std::string encoded_query = url_encode(query);
    std::string url = base_url_ + "/search/repositories?q=" + encoded_query +
                     "&per_page=" + std::to_string(max_results);

    // Make request
    cpr::Response r = cpr::Get(cpr::Url{url}, headers);

    // Check status
    if (r.status_code != 200) {
        spdlog::error("Search failed with status {}", r.status_code);
        return repositories;  // Return empty vector
    }

    // Parse JSON response
    nlohmann::json response = nlohmann::json::parse(r.text);

    // Extract repositories from "items" array
    if (response.contains("items")) {
        for (const auto& item : response["items"]) {
            Repository repo;
            repo.owner = item["owner"]["login"];
            repo.name = item["name"];
            repo.url = item["html_url"];
            repo.stars = item["stargazers_count"];
            repo.language = item["language"].is_null() ? "" : item["language"];

            repositories.push_back(repo);  // Add to vector
        }
    }

    spdlog::info("Found {} repositories", repositories.size());
    return repositories;
}

} // namespace overwatch
