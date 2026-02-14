#include "github_client.h"
#include <cpr/cpr.h>
#include <spdlog/spdlog.h>
#include <thread>
#include <chrono>
#include <ctime>
#include <stdexcept>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cctype>

using json = nlohmann::json;

namespace overwatch {

// Helper function to URL encode a string
std::string url_encode(const std::string& value) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (char c : value) {
        // Keep alphanumeric and other accepted characters intact
        if (isalnum(static_cast<unsigned char>(c)) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
        } else {
            // Any other characters are percent-encoded
            escaped << std::uppercase;
            escaped << '%' << std::setw(2) << int(static_cast<unsigned char>(c));
            escaped << std::nouppercase;
        }
    }

    return escaped.str();
}

// Helper function to decode base64
std::string base64_decode(const std::string& encoded) {
    // Simple base64 decoding using cpr's built-in utility or manual implementation
    static const std::string base64_chars =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "abcdefghijklmnopqrstuvwxyz"
        "0123456789+/";

    std::string decoded;
    std::vector<int> T(256, -1);
    for (int i = 0; i < 64; i++) T[base64_chars[i]] = i;

    int val = 0, valb = -8;
    for (unsigned char c : encoded) {
        if (T[c] == -1) break;
        val = (val << 6) + T[c];
        valb += 6;
        if (valb >= 0) {
            decoded.push_back(char((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return decoded;
}

int RateLimit::seconds_until_reset(int current_time) const {
    return std::max(0, reset_timestamp - current_time);
}

GitHubClient::GitHubClient(const std::string& token)
    : token_(token), base_url_("https://api.github.com") {
    if (token_.empty()) {
        spdlog::warn("No GitHub token provided. Rate limits will be much lower (60/hour vs 5000/hour)");
    } else {
        spdlog::info("GitHub client initialized with authentication token");
    }
}

json GitHubClient::makeRequest(const std::string& endpoint, const std::string& accept_header) {
    std::string url = base_url_ + endpoint;

    // Setup headers
    cpr::Header headers = {
        {"Accept", accept_header},
        {"User-Agent", "OverWatch-Scanner/0.1"}
    };

    if (!token_.empty()) {
        headers["Authorization"] = "Bearer " + token_;
    }

    spdlog::debug("Making request to: {}", url);

    // Make the request
    cpr::Response r = cpr::Get(
        cpr::Url{url},
        headers
    );

    // Log response details
    spdlog::debug("Response status: {}", r.status_code);
    spdlog::debug("Response body length: {}", r.text.length());
    if (r.text.length() < 500) {
        spdlog::debug("Response body: {}", r.text);
    }

    // Check for errors
    if (r.status_code >= 400) {
        spdlog::error("API request failed: {} - {}", r.status_code, r.text);
        throw std::runtime_error("GitHub API request failed: " + std::to_string(r.status_code));
    }

    // Update rate limit from headers
    std::map<std::string, std::string> header_map;
    for (const auto& h : r.header) {
        header_map[h.first] = h.second;
    }
    updateRateLimitFromHeaders(header_map);

    // Parse JSON response
    try {
        return json::parse(r.text);
    } catch (const json::parse_error& e) {
        spdlog::error("Failed to parse JSON response: {}", e.what());
        throw;
    }
}

void GitHubClient::updateRateLimitFromHeaders(const std::map<std::string, std::string>& headers) {
    auto it_limit = headers.find("x-ratelimit-limit");
    auto it_remaining = headers.find("x-ratelimit-remaining");
    auto it_reset = headers.find("x-ratelimit-reset");

    if (it_limit != headers.end() && it_remaining != headers.end() && it_reset != headers.end()) {
        RateLimit rl;
        rl.limit = std::stoi(it_limit->second);
        rl.remaining = std::stoi(it_remaining->second);
        rl.reset_timestamp = std::stoi(it_reset->second);
        cached_rate_limit_ = rl;

        spdlog::debug("Rate limit: {}/{} remaining", rl.remaining, rl.limit);

        if (rl.is_exhausted()) {
            spdlog::warn("Rate limit is low: {} remaining", rl.remaining);
        }
    }
}

RateLimit GitHubClient::getRateLimit() {
    if (cached_rate_limit_.has_value()) {
        return cached_rate_limit_.value();
    }

    // Fetch rate limit from API
    try {
        json response = makeRequest("/rate_limit");
        auto rate = response["rate"];

        RateLimit rl;
        rl.limit = rate["limit"];
        rl.remaining = rate["remaining"];
        rl.reset_timestamp = rate["reset"];

        cached_rate_limit_ = rl;
        return rl;
    } catch (const std::exception& e) {
        spdlog::error("Failed to get rate limit: {}", e.what());
        // Return a conservative estimate
        return RateLimit{60, 10, static_cast<int>(std::time(nullptr)) + 3600};
    }
}

bool GitHubClient::checkAndHandleRateLimit() {
    RateLimit rl = getRateLimit();

    if (rl.is_exhausted()) {
        int current_time = static_cast<int>(std::time(nullptr));
        int wait_seconds = rl.seconds_until_reset(current_time);

        spdlog::warn("Rate limit exhausted. Sleeping for {} seconds until reset...", wait_seconds);
        std::this_thread::sleep_for(std::chrono::seconds(wait_seconds + 5)); // +5 for safety margin
        return true;
    }

    return false;
}

std::vector<Repository> GitHubClient::searchRepositories(const std::string& query, int max_results) {
    std::vector<Repository> repositories;

    int per_page = std::min(100, max_results); // GitHub max is 100 per page
    int total_needed = max_results;
    int page = 1;

    while (repositories.size() < static_cast<size_t>(total_needed)) {
        checkAndHandleRateLimit();

        // URL encode the query
        std::string encoded_query = url_encode(query);

        std::string endpoint = "/search/repositories?q=" + encoded_query +
                              "&per_page=" + std::to_string(per_page) +
                              "&page=" + std::to_string(page);

        try {
            json response = makeRequest(endpoint);

            int total_count = response["total_count"];
            spdlog::info("Found {} repositories matching query (page {})", total_count, page);

            if (!response.contains("items") || response["items"].empty()) {
                spdlog::info("No more repositories found");
                break;
            }

            for (const auto& item : response["items"]) {
                Repository repo;
                repo.owner = item["owner"]["login"];
                repo.name = item["name"];
                repo.full_name = item["full_name"];
                repo.url = item["html_url"];
                repo.stars = item["stargazers_count"];
                repo.created_at = item["created_at"];
                repo.language = item["language"].is_null() ? "" : item["language"];

                repositories.push_back(repo);

                if (repositories.size() >= static_cast<size_t>(total_needed)) {
                    break;
                }
            }

            page++;

        } catch (const std::exception& e) {
            spdlog::error("Error searching repositories: {}", e.what());
            break;
        }
    }

    spdlog::info("Retrieved {} repositories", repositories.size());
    return repositories;
}

std::optional<std::string> GitHubClient::getFileContents(
    const std::string& owner,
    const std::string& repo,
    const std::string& path
) {
    checkAndHandleRateLimit();

    std::string endpoint = "/repos/" + owner + "/" + repo + "/contents/" + path;

    try {
        json response = makeRequest(endpoint);

        if (!response.contains("content") || response["content"].is_null()) {
            spdlog::warn("File {} has no content", path);
            return std::nullopt;
        }

        std::string encoded_content = response["content"];

        // Remove newlines from base64 string
        encoded_content.erase(
            std::remove(encoded_content.begin(), encoded_content.end(), '\n'),
            encoded_content.end()
        );

        // Decode base64
        std::string decoded = base64_decode(encoded_content);

        spdlog::debug("Retrieved file {} ({} bytes)", path, decoded.size());
        return decoded;

    } catch (const std::exception& e) {
        spdlog::debug("Could not retrieve file {}: {}", path, e.what());
        return std::nullopt;
    }
}

std::vector<FileInfo> GitHubClient::searchCode(const std::string& query, int max_results) {
    std::vector<FileInfo> files;

    checkAndHandleRateLimit();

    // URL encode the query
    std::string encoded_query = url_encode(query);

    int per_page = std::min(100, max_results);
    std::string endpoint = "/search/code?q=" + encoded_query +
                          "&per_page=" + std::to_string(per_page);

    try {
        json response = makeRequest(endpoint);

        int total_count = response["total_count"];
        spdlog::info("Found {} files matching code search query", total_count);

        if (!response.contains("items") || response["items"].empty()) {
            return files;
        }

        for (const auto& item : response["items"]) {
            FileInfo file;
            file.path = item["path"];
            file.url = item["html_url"];
            file.sha = item["sha"];
            file.size = item.value("size", 0);

            files.push_back(file);

            if (files.size() >= static_cast<size_t>(max_results)) {
                break;
            }
        }

    } catch (const std::exception& e) {
        spdlog::error("Error searching code: {}", e.what());
    }

    spdlog::info("Retrieved {} files", files.size());
    return files;
}

} // namespace overwatch
