#include "cli.h"
#include <spdlog/spdlog.h>
#include <iostream>
#include <cstdlib>

namespace overwatch {

CLI::CLI(int argc, char* argv[]) : argc_(argc), argv_(argv) {
}

Command CLI::stringToCommand(const std::string& cmd) {
    if (cmd == "run") return Command::RUN;
    if (cmd == "add") return Command::ADD;
    if (cmd == "delete") return Command::DELETE_;
    if (cmd == "all") return Command::ALL;
    if (cmd == "random") return Command::RANDOM;
    if (cmd == "filter") return Command::FILTER;
    if (cmd == "list") return Command::LIST;
    if (cmd == "help" || cmd == "--help" || cmd == "-h") return Command::HELP;
    return Command::UNKNOWN;
}

void CLI::parse() {
    if (argc_ < 2) {
        showHelp();
        std::exit(0);
    }

    // First argument is the command
    command_ = stringToCommand(argv_[1]);

    // Parse remaining arguments
    for (int i = 2; i < argc_; i++) {
        std::string arg = argv_[i];

        if (arg.substr(0, 2) == "--") {
            // It's a flag like --tag python
            std::string flag = arg.substr(2);

            // Check if there's a value after this flag
            if (i + 1 < argc_ && argv_[i + 1][0] != '-') {
                options_[flag] = argv_[i + 1];
                i++;  // Skip next arg since we consumed it
            } else {
                options_[flag] = "true";  // Boolean flag
            }
        } else {
            // Positional argument
            positional_args_.push_back(arg);
        }
    }
}

int CLI::execute() {
    switch (command_) {
        case Command::RUN:
            return runCommand();

        case Command::ADD:
            return addCommand();

        case Command::DELETE_:
            return deleteCommand();

        case Command::ALL:
            return allCommand();

        case Command::RANDOM:
            return randomCommand();

        case Command::FILTER:
            return filterCommand();

        case Command::LIST:
            return listCommand();

        case Command::HELP:
            showHelp();
            return 0;

        case Command::UNKNOWN:
        default:
            spdlog::error("Unknown command");
            spdlog::info("Run 'overwatch help' for usage information");
            return 1;
    }
}

int CLI::runCommand() {
    if (positional_args_.empty()) {
        spdlog::error("No query provided");
        spdlog::info("Usage: overwatch run \"language:Python stars:<5\"");
        return 1;
    }

    std::string query_str = positional_args_[0];
    int max_repos = 5;  // Default

    if (options_.count("max-repos")) {
        max_repos = std::stoi(options_["max-repos"]);
    }

    Query query;
    query.id = 0;  // Temporary query, no ID needed
    query.name = "CLI Query";
    query.query = query_str;
    query.max_repos = max_repos;

    runScan(query);
    return 0;
}

int CLI::addCommand() {
    if (!options_.count("name") || !options_.count("query")) {
        spdlog::error("Missing required arguments");
        spdlog::info("Usage: overwatch add --name \"Query Name\" --query \"language:Python\" --tag python --max-repos 5");
        return 1;
    }

    QueryBank bank;
    bank.load("data/query_bank.yaml");

    Query query;
    query.id = bank.getNextId();
    query.name = options_["name"];
    query.query = options_["query"];
    query.max_repos = options_.count("max-repos") ? std::stoi(options_["max-repos"]) : 5;

    // Parse tags (can have multiple --tag flags, but for simplicity we'll just take one)
    if (options_.count("tag")) {
        query.tags.push_back(options_["tag"]);
    }

    bank.addQuery(query);
    bank.save("data/query_bank.yaml");

    spdlog::info("Added query '{}' with ID {}", query.name, query.id);
    return 0;
}

int CLI::deleteCommand() {
    if (positional_args_.empty()) {
        spdlog::error("No query ID provided");
        spdlog::info("Usage: overwatch delete <id>");
        return 1;
    }

    int id = std::stoi(positional_args_[0]);

    QueryBank bank;
    bank.load("data/query_bank.yaml");

    if (bank.deleteQuery(id)) {
        bank.save("data/query_bank.yaml");
        spdlog::info("Query deleted successfully");
        return 0;
    } else {
        spdlog::error("Query with ID {} not found", id);
        return 1;
    }
}

int CLI::allCommand() {
    QueryBank bank;
    bank.load("data/query_bank.yaml");

    auto queries = bank.getAllQueries();

    if (queries.empty()) {
        spdlog::warn("Query bank is empty");
        return 0;
    }

    spdlog::info("Running all {} queries from bank", queries.size());

    for (const auto& query : queries) {
        spdlog::info("\n--- Running: {} ---", query.name);
        runScan(query);
    }

    return 0;
}

int CLI::randomCommand() {
    QueryBank bank;
    bank.load("data/query_bank.yaml");

    try {
        Query query = bank.getRandomQuery();
        spdlog::info("Randomly selected: {}", query.name);
        runScan(query);
        return 0;
    } catch (const std::exception& e) {
        spdlog::error("Error: {}", e.what());
        return 1;
    }
}

int CLI::filterCommand() {
    if (!options_.count("tag")) {
        spdlog::error("No tag provided");
        spdlog::info("Usage: overwatch filter --tag python");
        return 1;
    }

    std::string tag = options_["tag"];

    QueryBank bank;
    bank.load("data/query_bank.yaml");

    auto queries = bank.filterByTag(tag);

    if (queries.empty()) {
        spdlog::warn("No queries found with tag: {}", tag);
        return 0;
    }

    spdlog::info("Found {} queries with tag '{}'", queries.size(), tag);

    for (const auto& query : queries) {
        spdlog::info("\n--- Running: {} ---", query.name);
        runScan(query);
    }

    return 0;
}

int CLI::listCommand() {
    QueryBank bank;
    bank.load("data/query_bank.yaml");

    auto queries = bank.getAllQueries();

    if (queries.empty()) {
        spdlog::info("Query bank is empty");
        return 0;
    }

    spdlog::info("Query Bank ({} queries):\n", queries.size());

    for (const auto& query : queries) {
        std::cout << "  [" << query.id << "] " << query.name << "\n";
        std::cout << "      Query: " << query.query << "\n";
        std::cout << "      Tags: ";
        for (size_t i = 0; i < query.tags.size(); i++) {
            std::cout << query.tags[i];
            if (i < query.tags.size() - 1) std::cout << ", ";
        }
        std::cout << "\n";
        std::cout << "      Max repos: " << query.max_repos << "\n\n";
    }

    return 0;
}

void CLI::runScan(const Query& query) {
    // Get GitHub token
    const char* token_env = std::getenv("GITHUB_TOKEN");
    std::string token = token_env ? token_env : "";

    if (token.empty()) {
        spdlog::warn("No GITHUB_TOKEN found - using unauthenticated API (lower rate limits)");
        spdlog::warn("Set your token: export GITHUB_TOKEN=\"ghp_...\"");
    }

    // Create GitHub client
    GitHubClient client(token);

    // Validate token if provided
    if (!token.empty() && !client.validateToken()) {
        spdlog::error("Failed to validate GitHub token. Please check:");
        spdlog::error("  1. Token is not expired: https://github.com/settings/tokens");
        spdlog::error("  2. Token has 'public_repo' scope");
        spdlog::error("  3. Token format is correct (starts with ghp_)");
        throw std::runtime_error("Invalid GitHub token");
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

    if (remaining < 20) {
        spdlog::warn("Low on API quota! Only {} requests remaining", remaining);
    }

    // Create scanner components
    SecretDetector detector;
    detector.loadPatterns("config/patterns.yaml");
    Scanner scanner(client, detector, "data/findings.jsonl");

    // Run scan
    spdlog::info("Starting scan: {}", query.name.empty() ? query.query : query.name);
    scanner.run(query.query, query.max_repos);
    spdlog::info("Scan complete!");
}

void CLI::showHelp() {
    std::cout << "OverWatch Scanner - GitHub Secret Scanner\n\n";
    std::cout << "USAGE:\n";
    std::cout << "  overwatch <command> [options]\n\n";
    std::cout << "COMMANDS:\n";
    std::cout << "  run <query>              Run a single query\n";
    std::cout << "  add                      Add query to bank\n";
    std::cout << "  delete <id>              Delete query from bank\n";
    std::cout << "  list                     List all queries in bank\n";
    std::cout << "  all                      Run all queries from bank\n";
    std::cout << "  random                   Run a random query from bank\n";
    std::cout << "  filter --tag <tag>       Run queries with specific tag\n";
    std::cout << "  help                     Show this help message\n\n";
    std::cout << "EXAMPLES:\n";
    std::cout << "  overwatch run \"language:Python stars:<5\"\n";
    std::cout << "  overwatch run \"language:Python stars:<5\" --max-repos 10\n";
    std::cout << "  overwatch add --name \"Low Star Python\" --query \"language:Python stars:<5\" --tag python --max-repos 5\n";
    std::cout << "  overwatch delete 3\n";
    std::cout << "  overwatch list\n";
    std::cout << "  overwatch all\n";
    std::cout << "  overwatch random\n";
    std::cout << "  overwatch filter --tag python\n\n";
}

} 
