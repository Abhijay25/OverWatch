#pragma once

#include "github_client.h"
#include "secret_detector.h"
#include <string>
#include <vector>
#include <unordered_set>

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
    std::string scanned_repos_file_;
    std::unordered_set<std::string> scanned_repos_;

    // List of suspicious filenames to check
    std::vector<std::string> suspicious_files_ = {
        ".env",
        ".env.local",
        ".env.production",
        ".env.example",
        ".env.dev",
        "config.json",
        "config.yaml",
        "config.yml",
        "config.py",
        "settings.py",
        "secrets.json",
        "credentials.json",
        "token.txt",
        "tokens.txt",
        "credentials.txt",
        "auth.json",
        "google-services.json",
        "GoogleService-Info.plist",
        "firebase.json",
        "appsettings.json",
        ".npmrc",
        ".pypirc",
        "bot_config.json",
        "bot.config"
    };

    void scanRepository(const Repository& repo);
    void writeFinding(const std::string& owner, const std::string& repo,
                     const std::string& file, const Match& match);
    void loadScannedRepos();
    void saveScannedRepo(const std::string& owner, const std::string& repo);
    bool isAlreadyScanned(const std::string& owner, const std::string& repo);
};

} // namespace overwatch
