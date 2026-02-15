#include <spdlog/spdlog.h>
#include "cli.h"

int main(int argc, char* argv[]) {
    // Set logging level
    spdlog::set_level(spdlog::level::info);

    try {
        // Create CLI and parse arguments
        overwatch::CLI cli(argc, argv);
        cli.parse();

        // Execute the command
        return cli.execute();

    } catch (const std::exception& e) {
        spdlog::error("Error: {}", e.what());
        return 1;
    }
}
