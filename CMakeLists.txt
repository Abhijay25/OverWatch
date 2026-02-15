cmake_minimum_required(VERSION 3.15)
project(OverWatch VERSION 1.0.0)

# Build the C++ scanner
add_subdirectory(scanner)

# Note: Python bot doesn't need CMake
# Install instructions for bot are in bot/README.md
