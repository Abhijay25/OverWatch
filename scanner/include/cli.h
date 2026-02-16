#pragma once

#include "github_client.h"
#include "secret_detector.h"
#include "scanner.h"
#include "query_bank.h"
#include <string>
#include <map>

namespace overwatch {

/**
 * Command types
 */
enum class Command {
    RUN,
    ADD,
    DELETE_,  // DELETE is a reserved word in some systems
    ALL,
    RANDOM,
    CONTINUOUS,
    FILTER,
    LIST,
    HELP,
    UNKNOWN
};

/**
 * Command-line interface for the scanner
 * Handles different commands: run, add, delete, all, random, filter
 */
class CLI {
public:
    CLI(int argc, char* argv[]);

    /**
     * Parse command-line arguments
     */
    void parse();

    /**
     * Execute the parsed command
     */
    int execute();

private:
    int argc_;
    char** argv_;

    Command command_;  // Parsed command enum
    std::map<std::string, std::string> options_;  // --tag, --name, etc.
    std::vector<std::string> positional_args_;    // Non-flag arguments

    // Helper to convert string to enum
    Command stringToCommand(const std::string& cmd);

    // Command handlers
    int runCommand();
    int addCommand();
    int deleteCommand();
    int allCommand();
    int randomCommand();
    int continuousCommand();
    int filterCommand();
    int listCommand();

    // Helpers
    void showHelp();
    void runScan(const Query& query);
    void runScanNoValidate(const Query& query, const std::string& token, SecretDetector& detector);
};

} // namespace overwatch
