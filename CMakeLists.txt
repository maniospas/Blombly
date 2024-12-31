# Minimum CMake version required
cmake_minimum_required(VERSION 3.16)

# Set vcpkg toolchain
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")

set(CMAKE_C_COMPILER "gcc")
set(CMAKE_CXX_COMPILER "g++")

# Project name and version
project(blombly VERSION 3.1 LANGUAGES CXX)

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fexec-charset=UTF-8 -finput-charset=UTF-8 -O2 -s")
#add_definitions(-D_WIN32_WINNT=0x0A00)

# Include directories: add your project's include folder
include_directories(${PROJECT_SOURCE_DIR}/include)

# Collect all .cpp source files from src/
file(GLOB_RECURSE SOURCES
    "${PROJECT_SOURCE_DIR}/blombly.cpp"
    "${PROJECT_SOURCE_DIR}/src/*.cpp"
)

# Create the executable from the collected source files
add_executable(${PROJECT_NAME} ${SOURCES})

# Find and link zlib for compression support
find_package(ZLIB REQUIRED)

# Manually include Civetweb headers and link library (installed via vcpkg)
find_path(CIVETWEB_INCLUDE_DIR civetweb.h)
find_library(CIVETWEB_LIBRARY civetweb)

# Include the Civetweb directory and link the library
if(CIVETWEB_INCLUDE_DIR AND CIVETWEB_LIBRARY)
    target_include_directories(${PROJECT_NAME} PRIVATE ${CIVETWEB_INCLUDE_DIR})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${CIVETWEB_LIBRARY})
else()
    message(FATAL_ERROR "Civetweb library not found. Make sure it is installed via vcpkg.")
endif()

# Include zlib directory and link the necessary libraries
if(ZLIB_FOUND)
    target_include_directories(${PROJECT_NAME} PRIVATE ${ZLIB_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} PRIVATE ${ZLIB_LIBRARIES})
else()
    message(FATAL_ERROR "Zlib library not found.")
endif()

find_package(CURL REQUIRED)

# Link your executable or library with libcurl
target_link_libraries(${PROJECT_NAME} PRIVATE CURL::libcurl)


# Specify where to place the final executable
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}"
)

# Specify where to place intermediate and library files
set_target_properties(${PROJECT_NAME} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/build/output"
    LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/build/output"
)

# Specify where to place object files
set(CMAKE_OBJECT_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/build/output")

# Ensure the 'temp' directory exists
file(MAKE_DIRECTORY "${PROJECT_BINARY_DIR}/build/output")

# Set up build types
set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Limited configurations" FORCE)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/build/output"
    LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/build/output"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${PROJECT_SOURCE_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${PROJECT_BINARY_DIR}/build/output"
)
