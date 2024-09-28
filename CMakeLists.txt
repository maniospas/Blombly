# Minimum CMake version required
cmake_minimum_required(VERSION 3.16)

set(CMAKE_C_COMPILER "gcc")
set(CMAKE_CXX_COMPILER "g++")

# Project name and version
project(blombly VERSION 3.0 LANGUAGES CXX)

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler flags
# -O2: Optimization level 2
# -g: Include debugging information
# -fdiagnostics-color=always: Enable colored diagnostics
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")

# Include directories
# Add the 'include' directory to the list of paths to search for header files
include_directories(${PROJECT_SOURCE_DIR}/include)

# Collect all .cpp source files from src/
file(GLOB_RECURSE SOURCES
    "${PROJECT_SOURCE_DIR}/blombly.cpp"
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
    ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/build/output"
    LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/build/output"
)

# Specify where to place object files
# OBJECT_OUTPUT_DIRECTORY directs object files (.o, .obj) to 'temp'
set(CMAKE_OBJECT_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/build/output")

# Ensure the 'temp' directory exists
file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/build/output")

set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Limited configurations" FORCE)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/build/output"
    LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/build/output"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${PROJECT_SOURCE_DIR}"  # Release executable to top folder
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${PROJECT_BINARY_DIR}/build/output"  # Debug executable to temp
    RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${PROJECT_BINARY_DIR}/build/output"
    RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${PROJECT_BINARY_DIR}/build/output"
)