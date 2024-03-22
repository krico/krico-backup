cmake_minimum_required(VERSION 3.22)
project(krico-backup VERSION 1.0.0 DESCRIPTION "Space-efficient backups for personal use" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_C_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_TESTS "Enable building and running of unit tests" ON)
option(GENERATE_DOCS "Enable generation of documentation" ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(Version)

if (GENERATE_DOCS)
    message(STATUS "Api doc generation enabled (GENERATE_DOCS=ON)")
    add_custom_target(apidoc DEPENDS ${GENERATE_DOCS_TARGETS})
else (GENERATE_DOCS)
    message(STATUS "Api doc generation DISABLED (GENERATE_DOCS=OFF)")
endif (GENERATE_DOCS)

add_subdirectory(lib)
add_subdirectory(main)

if (BUILD_TESTS)
    message(STATUS "Tests are enabled (BUILD_TESTS=ON)")
    add_subdirectory(lib/tests)
else (BUILD_TESTS)
    message(STATUS "Tests are DISABLED (BUILD_TESTS=OFF)")
endif (BUILD_TESTS)
