cmake_minimum_required(VERSION 3.15)
project(OverWatch-Scanner VERSION 0.1.0 LANGUAGES CXX)

# Set C++17 standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find packages (try CONFIG first for vcpkg, fallback to MODULE for system packages)
find_package(cpr CONFIG)
if(NOT cpr_FOUND)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(cpr REQUIRED IMPORTED_TARGET libcpr)
    add_library(cpr::cpr ALIAS PkgConfig::cpr)
endif()

find_package(nlohmann_json CONFIG)
if(NOT nlohmann_json_FOUND)
    find_package(nlohmann_json REQUIRED)
endif()

find_package(yaml-cpp CONFIG)
if(NOT yaml-cpp_FOUND)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(yaml-cpp REQUIRED IMPORTED_TARGET yaml-cpp)
    add_library(yaml-cpp::yaml-cpp ALIAS PkgConfig::yaml-cpp)
endif()

find_package(spdlog CONFIG)
if(NOT spdlog_FOUND)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(spdlog REQUIRED IMPORTED_TARGET spdlog)
    add_library(spdlog::spdlog ALIAS PkgConfig::spdlog)
endif()

# Include directories
include_directories(include)

# Source files
set(SOURCES
    src/main.cpp
    src/github_client.cpp
    src/secret_detector.cpp
    src/output.cpp
)

# Create executable
add_executable(scanner ${SOURCES})

# Link libraries
target_link_libraries(scanner PRIVATE
    cpr::cpr
    nlohmann_json::nlohmann_json
    yaml-cpp::yaml-cpp
    spdlog::spdlog
)

# Compiler warnings
if(MSVC)
    target_compile_options(scanner PRIVATE /W4)
else()
    target_compile_options(scanner PRIVATE -Wall -Wextra -Wpedantic)
endif()
