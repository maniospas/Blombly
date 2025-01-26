# SETTINGS
# vcpkg integrate install
cmake_minimum_required(VERSION 3.16)
project(blombly VERSION 3.1 LANGUAGES CXX)
set(CMAKE_TOOLCHAIN_FILE "${CMAKE_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake" CACHE STRING "Vcpkg toolchain file")
set(CMAKE_CXX_COMPILER g++)
set(CMAKE_C_COMPILER gcc)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/vcpkg/installed/x64-mingw-static")

# FILES
include_directories(${PROJECT_SOURCE_DIR}/include)
file(GLOB_RECURSE SOURCES
    "${PROJECT_SOURCE_DIR}/blombly.cpp"
    "${PROJECT_SOURCE_DIR}/src/*.cpp"
)

# TARGET
# either -pg for profile or -s to strip symbols. profile obtained with gprof ./build/blombly gmon.out > profile_report.txt
add_executable(${PROJECT_NAME} ${SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/build/output"
    LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/build/output"
    RUNTIME_OUTPUT_DIRECTORY_RELEASE "${PROJECT_SOURCE_DIR}"
    RUNTIME_OUTPUT_DIRECTORY_DEBUG "${PROJECT_BINARY_DIR}/build/output"
	CMAKE_OBJECT_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/build/output"
)
add_definitions(-DUNICODE -D_UNICODE)
target_compile_options(${PROJECT_NAME} PRIVATE -fopenmp -fexec-charset=UTF-8 -finput-charset=UTF-8 -O2 -s)

# PACKAGES
find_package(unofficial-brotli CONFIG REQUIRED)
find_package(BZip2 REQUIRED)
find_package(SDL2 CONFIG REQUIRED)
find_package(SDL2_image CONFIG REQUIRED)
find_package(SDL2_ttf CONFIG REQUIRED)
find_package(ZLIB REQUIRED)
find_package(civetweb CONFIG REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(CURL REQUIRED)
find_package(unofficial-sqlite3 CONFIG REQUIRED)

target_link_libraries(${PROJECT_NAME}
	PRIVATE
	$<TARGET_NAME_IF_EXISTS:SDL2::SDL2main>
	$<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static>
)

target_link_libraries(${PROJECT_NAME} PRIVATE unofficial::brotli::brotlienc)
target_link_libraries(${PROJECT_NAME} PRIVATE BZip2::BZip2)
target_link_libraries(${PROJECT_NAME} PRIVATE $<IF:$<TARGET_EXISTS:SDL2_image::SDL2_image>,SDL2_image::SDL2_image,SDL2_image::SDL2_image-static>)
target_link_libraries(${PROJECT_NAME} PRIVATE $<IF:$<TARGET_EXISTS:SDL2_ttf::SDL2_ttf>,SDL2_ttf::SDL2_ttf,SDL2_ttf::SDL2_ttf-static>)
target_link_libraries(${PROJECT_NAME} PRIVATE ZLIB::ZLIB)
target_link_libraries(${PROJECT_NAME} PRIVATE civetweb::civetweb)
target_link_libraries(${PROJECT_NAME} PRIVATE OpenSSL::SSL)
target_link_libraries(${PROJECT_NAME} PRIVATE OpenSSL::Crypto)
target_link_libraries(${PROJECT_NAME} PRIVATE CURL::libcurl)
target_link_libraries(${PROJECT_NAME} PRIVATE unofficial::sqlite3::sqlite3)