cmake_minimum_required(VERSION 3.15)
project(OverWatch-Scanner VERSION 0.1.0)

# Use C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find packages (Nix/vcpkg will provide these)
find_package(cpr CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(yaml-cpp CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)

# Source files
add_executable(scanner
    src/main.cpp
    # Add more .cpp files here as we create them
)

# Link libraries
target_link_libraries(scanner PRIVATE
    cpr::cpr
    nlohmann_json::nlohmann_json
    yaml-cpp::yaml-cpp
    spdlog::spdlog
)

# Enable warnings
if(MSVC)
    target_compile_options(scanner PRIVATE /W4)
else()
    target_compile_options(scanner PRIVATE -Wall -Wextra -Wpedantic)
endif()
