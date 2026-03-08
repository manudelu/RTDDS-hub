cmake_minimum_required(VERSION 3.16)
project(RTDDS_Hub VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# CMake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# Global include directories
include_directories(
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/generated/idl
)

add_subdirectory(src)

# Install runtime scripts
install(DIRECTORY scripts/
    DESTINATION bin
    USE_SOURCE_PERMISSIONS
)