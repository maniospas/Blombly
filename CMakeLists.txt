# Minimum CMake version required
cmake_minimum_required(VERSION 3.16)

# Set vcpkg toolchain
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")

# Force usage of GCC and G++
if(WIN32)
    set(CMAKE_C_COMPILER "gcc" CACHE STRING "GCC compiler" FORCE)
    set(CMAKE_CXX_COMPILER "g++" CACHE STRING "G++ compiler" FORCE)
endif()

# Project name and version
project(blombly VERSION 3.1 LANGUAGES CXX)

# Set C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fopenmp -fexec-charset=UTF-8 -finput-charset=UTF-8 -O2 -s")  # either -pg for profile or -s to strip symbols. profile obtained with gprof ./build/blombly gmon.out > profile_report.txt
add_definitions(-DUNICODE -D_UNICODE)

if(MSVC)
    string(REPLACE "-fexec-charset=UTF-8" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    string(REPLACE "-finput-charset=UTF-8" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    string(REPLACE "-s" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
endif()

# Include directories: add your project's include folder
include_directories(${PROJECT_SOURCE_DIR}/include)

# Treat all source files as UTF-8
add_compile_options("$<$<C_COMPILER_ID:GNU>:-finput-charset=UTF-8>")
add_compile_options("$<$<CXX_COMPILER_ID:GNU>:-finput-charset=UTF-8>")

# Collect all .cpp source files from src/
file(GLOB_RECURSE SOURCES
    "${PROJECT_SOURCE_DIR}/blombly.cpp"
    "${PROJECT_SOURCE_DIR}/src/*.cpp"
)

# Create the executable from the collected source files
add_executable(${PROJECT_NAME} ${SOURCES})

# Find and link zlib for compression support
find_package(ZLIB REQUIRED)

# Include Civetweb headers and link library (installed via vcpkg)
find_path(CIVETWEB_INCLUDE_DIR civetweb.h)
find_library(CIVETWEB_LIBRARY civetweb)

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

# Find and link CURL
find_package(CURL REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE CURL::libcurl)

# Find and link OpenSSL
find_package(OpenSSL REQUIRED)
target_include_directories(${PROJECT_NAME} PRIVATE ${OPENSSL_INCLUDE_DIR})
target_link_libraries(${PROJECT_NAME} PRIVATE OpenSSL::SSL OpenSSL::Crypto)

# Find and link SQLite
find_package(SQLite3 REQUIRED)
if(SQLite3_FOUND)
    target_include_directories(${PROJECT_NAME} PRIVATE ${SQLite3_INCLUDE_DIRS})
    target_link_libraries(${PROJECT_NAME} PRIVATE SQLite::SQLite3)
else()
    message(FATAL_ERROR "SQLite3 library not found. Make sure it is installed via vcpkg.")
endif()

# Platform-specific configuration
if(UNIX AND NOT WIN32) # For Linux and macOS
    # Find and link SDL2
    find_package(SDL2 REQUIRED)
    if(SDL2_FOUND)
        target_include_directories(${PROJECT_NAME} PRIVATE ${SDL2_INCLUDE_DIRS})
        target_link_libraries(${PROJECT_NAME} PRIVATE SDL2::SDL2 SDL2::SDL2main)
    else()
        message(FATAL_ERROR "SDL2 library not found. Make sure it is installed on the system.")
    endif()

    # Use PkgConfig to find SDL2_image and SDL2_ttf
    find_package(PkgConfig REQUIRED)

    pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)
    if(SDL2_IMAGE_FOUND)
        target_include_directories(${PROJECT_NAME} PRIVATE ${SDL2_IMAGE_INCLUDE_DIRS})
        target_link_libraries(${PROJECT_NAME} PRIVATE ${SDL2_IMAGE_LIBRARIES})
    else()
        message(FATAL_ERROR "SDL2_image library not found. Make sure it is installed on the system.")
    endif()

    pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)
    if(SDL2_TTF_FOUND)
        target_include_directories(${PROJECT_NAME} PRIVATE ${SDL2_TTF_INCLUDE_DIRS})
        target_link_libraries(${PROJECT_NAME} PRIVATE ${SDL2_TTF_LIBRARIES})
    else()
        message(FATAL_ERROR "SDL2_ttf library not found. Make sure it is installed on the system.")
    endif()

elseif(WIN32) # For Windows using GCC
    # Find and link SDL2
    find_package(SDL2 CONFIG REQUIRED)
    if(TARGET SDL2::SDL2main AND TARGET SDL2::SDL2)
        target_link_libraries(${PROJECT_NAME} PRIVATE SDL2::SDL2main SDL2::SDL2)
    elseif(TARGET SDL2::SDL2-static)
        target_link_libraries(${PROJECT_NAME} PRIVATE SDL2::SDL2-static)
    else()
        message(FATAL_ERROR "SDL2 library not found. Ensure SDL2 is properly installed and accessible.")
    endif()

    # Find and link SDL2_image
    find_package(SDL2_image CONFIG REQUIRED)
    if(TARGET SDL2_image::SDL2_image)
        target_link_libraries(${PROJECT_NAME} PRIVATE SDL2_image::SDL2_image)
    elseif(TARGET SDL2_image::SDL2_image-static)
        target_link_libraries(${PROJECT_NAME} PRIVATE SDL2_image::SDL2_image-static)
    else()
        message(FATAL_ERROR "SDL2_image library not found. Ensure SDL2_image is properly installed and accessible.")
    endif()

    # Find and link SDL2_ttf
    find_package(SDL2_ttf CONFIG REQUIRED)
    if(TARGET SDL2_ttf::SDL2_ttf)
        target_link_libraries(${PROJECT_NAME} PRIVATE SDL2_ttf::SDL2_ttf)
    elseif(TARGET SDL2_ttf::SDL2_ttf-static)
        target_link_libraries(${PROJECT_NAME} PRIVATE SDL2_ttf::SDL2_ttf-static)
    else()
        message(FATAL_ERROR "SDL2_ttf library not found. Ensure SDL2_ttf is properly installed and accessible.")
    endif()
endif()

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
