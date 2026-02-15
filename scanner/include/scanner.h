#pragma once

#include "github_client.h"
#include "secret_detector.h"
#include <string>
#include <vector>

namespace overwatch {

class Scanner {
public:
    /**
     * Create a scanner
     * @param client GitHub API client
     * @param detector Secret detector with loaded patterns
     * @param output_file Path to output JSONL file
     */
    Scanner(GitHubClient& client, SecretDetector& detector, const std::string& output_file);

    /**
     * Run the scanner
     * @param search_query GitHub search query
     * @param max_repos Maximum number of repositories to scan
     */
    void run(const std::string& search_query, int max_repos);

private:
    GitHubClient& client_;
    SecretDetector& detector_;
    std::string output_file_;

    // List of suspicious filenames to check
    std::vector<std::string> suspicious_files_ = {
        ".env",
        ".env.local",
        ".env.production",
        "config.json",
        "config.yaml",
        "config.yml",
        "secrets.json",
        "credentials.json",
        "google-services.json",
        "GoogleService-Info.plist",
        "firebase.json",
        ".npmrc",
        ".pypirc"
    };

    void scanRepository(const Repository& repo);
    void writeFinding(const std::string& owner, const std::string& repo,
                     const std::string& file, const Match& match);
};

} // namespace overwatch
