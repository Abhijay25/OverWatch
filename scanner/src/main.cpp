#include <iostream>
#include <cstdlib>
#include <vector>
#include <spdlog/spdlog.h>
#include "github_client.h"
#include "secret_detector.h"
#include "output.h"

int main(int argc, char* argv[]) {
    // Set log level
    spdlog::set_level(spdlog::level::info);

    spdlog::info("╔═══════════════════════════════════════╗");
    spdlog::info("║   OverWatch Scanner v0.1.0 🔒       ║");
    spdlog::info("║   Secret Detection System            ║");
    spdlog::info("╚═══════════════════════════════════════╝\n");

    // Parse command line arguments
    int max_repos = 10;
    int max_stars = 10;
    bool dry_run = false;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "--max-repos" && i + 1 < argc) {
            max_repos = std::stoi(argv[++i]);
        } else if (arg == "--max-stars" && i + 1 < argc) {
            max_stars = std::stoi(argv[++i]);
        } else if (arg == "--dry-run") {
            dry_run = true;
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: " << argv[0] << " [options]\n";
            std::cout << "Options:\n";
            std::cout << "  --max-repos N     Maximum repositories to scan (default: 10)\n";
            std::cout << "  --max-stars N     Only scan repos with < N stars (default: 10)\n";
            std::cout << "  --dry-run         Don't write findings to CSV\n";
            std::cout << "  --help, -h        Show this help\n";
            return 0;
        }
    }

    spdlog::info("Configuration:");
    spdlog::info("  Max repos: {}", max_repos);
    spdlog::info("  Max stars: {}", max_stars);
    spdlog::info("  Dry run: {}\n", dry_run ? "yes" : "no");

    // Get GitHub token from environment
    const char* token_env = std::getenv("GITHUB_TOKEN");
    std::string token = token_env ? token_env : "";

    if (token.empty()) {
        spdlog::warn("⚠️  GITHUB_TOKEN not set!");
        spdlog::warn("Using unauthenticated API (60 requests/hour)");
        spdlog::info("For better rate limits (5000/hour), set GITHUB_TOKEN\n");
    } else {
        spdlog::info("✓ Authenticated with GitHub token\n");
    }

    // Create GitHub client
    overwatch::GitHubClient client(token);

    // Check rate limit
    try {
        auto rate_limit = client.getRateLimit();
        spdlog::info("Rate limit: {}/{} requests remaining", rate_limit.remaining, rate_limit.limit);
        if (rate_limit.remaining < 50) {
            spdlog::warn("⚠️  Low rate limit! Scanner may be throttled\n");
        }
    } catch (const std::exception& e) {
        spdlog::error("Failed to check rate limit: {}", e.what());
    }

    // Load secret patterns
    spdlog::info("\n📋 Loading secret detection patterns...");
    overwatch::SecretDetector detector;
    int pattern_count = detector.loadPatterns("../config/patterns.yaml");

    if (pattern_count == 0) {
        spdlog::error("No patterns loaded! Check config/patterns.yaml");
        return 1;
    }
    spdlog::info("✓ Loaded {} secret patterns\n", pattern_count);

    // Search for repositories
    spdlog::info("🔍 Searching for repositories...");
    std::string query = "language:Python stars:<" + std::to_string(max_stars) +
                       " created:>2026-02-07";
    spdlog::info("Query: {}", query);

    std::vector<overwatch::Repository> repos;
    try {
        repos = client.searchRepositories(query, max_repos);
        spdlog::info("✓ Found {} repositories\n", repos.size());
    } catch (const std::exception& e) {
        spdlog::error("Repository search failed: {}", e.what());
        return 1;
    }

    if (repos.empty()) {
        spdlog::info("No repositories found matching criteria");
        return 0;
    }

    // Scan each repository
    std::vector<overwatch::Finding> all_findings;
    int repos_scanned = 0;
    int files_scanned = 0;

    // Common files that might contain secrets
    std::vector<std::string> target_files = {
        ".env",
        "config.json",
        "config.yaml",
        "config.yml",
        "settings.py",
        "constants.py",
        "secrets.json"
    };

    for (const auto& repo : repos) {
        repos_scanned++;
        spdlog::info("\n[{}/{}] Scanning: {}/{}",
                    repos_scanned, repos.size(),
                    repo.owner, repo.name);

        // Try to get each target file
        for (const auto& file_path : target_files) {
            try {
                auto contents = client.getFileContents(repo.owner, repo.name, file_path);

                if (contents.has_value()) {
                    files_scanned++;
                    std::string content = contents.value();
                    spdlog::info("  📄 Scanning {} ({} bytes)", file_path, content.length());

                    // Scan for secrets
                    std::string file_url = repo.url + "/blob/main/" + file_path;
                    auto findings = detector.scanFile(
                        content,
                        file_path,
                        repo.owner,
                        repo.name,
                        file_path,
                        repo.url,
                        file_url
                    );

                    if (!findings.empty()) {
                        spdlog::warn("  ⚠️  Found {} secret(s) in {}", findings.size(), file_path);
                        for (const auto& f : findings) {
                            spdlog::warn("      - Line {}: {}", f.line_number, f.secret_type);
                        }
                        all_findings.insert(all_findings.end(), findings.begin(), findings.end());
                    }
                }
            } catch (const std::exception& e) {
                // File doesn't exist or couldn't be retrieved - that's fine
                spdlog::debug("  Couldn't get {}: {}", file_path, e.what());
            }
        }
    }

    // Summary
    spdlog::info("\n╔═══════════════════════════════════════╗");
    spdlog::info("║          Scan Summary                 ║");
    spdlog::info("╚═══════════════════════════════════════╝");
    spdlog::info("Repositories scanned: {}", repos_scanned);
    spdlog::info("Files scanned: {}", files_scanned);
    spdlog::info("Secrets found: {}\n", all_findings.size());

    // Write findings to CSV
    if (!all_findings.empty() && !dry_run) {
        spdlog::info("💾 Writing findings to ../data/findings.csv...");
        overwatch::CSVWriter writer("../data/findings.csv", true);
        int written = writer.writeFindings(all_findings);
        spdlog::info("✓ Wrote {} findings", written);
        spdlog::info("\nNext steps:");
        spdlog::info("1. Review: cat ../data/findings.csv");
        spdlog::info("2. Run bot: python ../bot/bot.py --input ../data/findings.csv --dry-run");
    } else if (dry_run) {
        spdlog::info("ℹ️  Dry run - no findings written");
    } else {
        spdlog::info("✓ No secrets found!");
    }

    spdlog::info("\n✅ Scanner completed successfully");
    return 0;
}
