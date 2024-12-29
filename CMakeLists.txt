# Minimum CMake version required
cmake_minimum_required(VERSION 3.16)

# Set project name and version
project(blombly VERSION 3.1 LANGUAGES CXX)

# Set vcpkg toolchain
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")

# Specify compilers
set(CMAKE_C_COMPILER "gcc")
set(CMAKE_CXX_COMPILER "g++")

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexec-charset=UTF-8 -finput-charset=UTF-8 -O2 -s")  # for debugging/tuning use -g for debug symbols, don't strip them with -s

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/include)

# Collect all source files
file(GLOB_RECURSE SOURCES
    "${PROJECT_SOURCE_DIR}/blombly.cpp"
    "${PROJECT_SOURCE_DIR}/src/*.cpp"
)

# Create the executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Find and link required libraries
## Zlib
find_package(ZLIB REQUIRED)
if(ZLIB_FOUND)
    target_include_directories(${PROJECT_NAME} PRIVATE ${ZLIB_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${ZLIB_LIBRARIES})
else()
    message(FATAL_ERROR "Zlib library not found.")
endif()

## Civetweb
find_path(CIVETWEB_INCLUDE_DIR civetweb.h)
find_library(CIVETWEB_LIBRARY civetweb)
if(CIVETWEB_INCLUDE_DIR AND CIVETWEB_LIBRARY)
    target_include_directories(${PROJECT_NAME} PRIVATE ${CIVETWEB_INCLUDE_DIR})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${CIVETWEB_LIBRARY})
else()
    message(FATAL_ERROR "Civetweb library not found. Make sure it is installed via vcpkg.")
endif()

## Curl
find_package(CURL REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE CURL::libcurl)

# Output directories
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}"
    ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/build/output"
    LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/build/output"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${PROJECT_SOURCE_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${PROJECT_BINARY_DIR}/build/output"
)

# Object file directory
set(CMAKE_OBJECT_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/build/output")

# Ensure output directory exists
file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/build/output")

# Build configurations
set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Limited configurations" FORCE)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Additional commented-out sections (e.g., asmjit, LLVM)
# These can be enabled as needed.
# asmjit
# include_directories(${PROJECT_SOURCE_DIR}/asmjit/src)

# LLVM
# find_package(LLVM REQUIRED CONFIG)
# include_directories(SYSTEM ${LLVM_INCLUDE_DIRS})
# add_definitions(${LLVM_DEFINITIONS})
# target_link_libraries(${PROJECT_NAME} PRIVATE LLVM)
