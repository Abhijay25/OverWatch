#include "scanner.h"
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <fstream>
#include <chrono>
#include <iomanip>
#include <sstream>

namespace overwatch {

Scanner::Scanner(GitHubClient& client, SecretDetector& detector, const std::string& output_file)
    : client_(client), detector_(detector), output_file_(output_file) {
}

void Scanner::run(const std::string& search_query, int max_repos) {
    spdlog::info("Starting scan with query: {}", search_query);
    if (max_repos == 0) {
        spdlog::info("Maximum repositories to scan: unlimited");
    } else {
        spdlog::info("Maximum repositories to scan: {}", max_repos);
    }

    // Search for repositories
    auto repos = client_.searchRepositories(search_query, max_repos);

    if (repos.empty()) {
        spdlog::warn("No repositories found matching query");
        return;
    }

    spdlog::info("Found {} repositories to scan", repos.size());

    // Scan each repository
    int scanned = 0;
    for (const auto& repo : repos) {
        spdlog::info("Scanning {}/{} ...", repo.owner, repo.name);
        scanRepository(repo);
        scanned++;
    }

    spdlog::info("Scan complete! Scanned {} repositories", scanned);
}

void Scanner::scanRepository(const Repository& repo) {
    // Try to fetch each suspicious file
    for (const auto& filename : suspicious_files_) {
        try {
            spdlog::debug("Checking for file: {}", filename);

            // Try to get file content
            std::string content = client_.getFileContent(repo.owner, repo.name, filename);

            spdlog::info("Found file: {} ({} bytes)", filename, content.size());

            // Scan content for secrets
            auto matches = detector_.scanContent(content, filename);

            if (!matches.empty()) {
                spdlog::warn("Found {} potential secrets in {}/{}/{}",
                           matches.size(), repo.owner, repo.name, filename);

                // Write each finding to output file
                for (const auto& match : matches) {
                    writeFinding(repo.owner, repo.name, filename, match);
                }
            }

        } catch (const std::exception& e) {
            // File doesn't exist or couldn't be fetched - that's OK, continue
            spdlog::debug("Could not fetch {}: {}", filename, e.what());
        }
    }
}

void Scanner::writeFinding(const std::string& owner, const std::string& repo,
                          const std::string& file, const Match& match) {
    // Get current timestamp
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::gmtime(&time_t), "%Y-%m-%dT%H:%M:%SZ");

    // Build JSON object
    nlohmann::json finding = {
        {"owner", owner},
        {"repo", repo},
        {"file", file},
        {"line", match.line_number},
        {"secret_type", match.pattern_name},
        {"matched_text", match.matched_text},
        {"timestamp", ss.str()}
    };

    // Append to JSONL file
    std::ofstream outfile(output_file_, std::ios::app);
    if (!outfile) {
        spdlog::error("Failed to open output file: {}", output_file_);
        return;
    }

    outfile << finding.dump() << "\n";
    outfile.close();

    spdlog::info("Wrote finding: {}/{}/{} line {} - {}",
                owner, repo, file, match.line_number, match.pattern_name);
}

} 
