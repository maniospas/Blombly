# Minimum CMake version required
cmake_minimum_required(VERSION 3.16)

# Project name and version
project(MyProject VERSION 1.0 LANGUAGES CXX)

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler flags
# -O2: Optimization level 2
# -g: Include debugging information
# -fdiagnostics-color=always: Enable colored diagnostics
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -g -fdiagnostics-color=always")

# Include directories
# Add the 'include' directory to the list of paths to search for header files
include_directories(${PROJECT_SOURCE_DIR}/include)

# Collect all .cpp source files from src/
file(GLOB_RECURSE SOURCES
    "${PROJECT_SOURCE_DIR}/src/*.cpp"
)

# Create the executable from the collected source files
add_executable(${PROJECT_NAME} ${SOURCES})

# Specify where to place the final executable
# RUNTIME_OUTPUT_DIRECTORY defines where the executable will be placed
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}"
)

# Specify where to place intermediate and library files
# ARCHIVE_OUTPUT_DIRECTORY: Static libraries (.a, .lib)
# LIBRARY_OUTPUT_DIRECTORY: Shared libraries (.so, .dll)
# Both are set to the 'temp' directory
set_target_properties(${PROJECT_NAME} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/temp"
    LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/temp"
)

# Specify where to place object files
# OBJECT_OUTPUT_DIRECTORY directs object files (.o, .obj) to 'temp'
set(CMAKE_OBJECT_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/temp")

# Ensure the 'temp' directory exists
file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/temp")

# Optional: Enable caching of compiled files
# CMake inherently caches build information to avoid unnecessary recompilations
# No additional configuration is typically required for basic caching
