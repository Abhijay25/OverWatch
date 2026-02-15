cmake_minimum_required(VERSION 3.15)
project(OverWatch-Scanner VERSION 0.1.0)

# Use C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Generate compile_commands.json for IDEs
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Find packages (Nix/vcpkg will provide these)
find_package(cpr CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

# Source files
add_executable(overwatch
    src/main.cpp
    src/github_client.cpp
    src/secret_detector.cpp
    src/scanner.cpp
    src/query_bank.cpp
    src/cli.cpp
    src/base64.cpp
)

# Tell compiler where to find our header files
target_include_directories(overwatch PRIVATE include)

# Link libraries
target_link_libraries(overwatch PRIVATE
    cpr::cpr
    nlohmann_json::nlohmann_json
    yaml-cpp::yaml-cpp
    spdlog::spdlog
)

# Enable warnings
if(MSVC)
    target_compile_options(overwatch PRIVATE /W4)
else()
    target_compile_options(overwatch PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Install target - installs to ~/.local/bin by default
install(TARGETS overwatch DESTINATION bin)
