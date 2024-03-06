cmake_minimum_required(VERSION 3.22)
project(krico-backup VERSION 1.0 DESCRIPTION "Space-efficient backups for personal use" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_TESTS "Enable building and running of unit tests" ON)
option(GENERATE_DOCS "Enable generation of documentation" ON)

add_subdirectory(lib)
add_subdirectory(main)

if (BUILD_TESTS)
    message(STATUS "Tests are enabled (BUILD_TESTS=ON)")
    add_subdirectory(lib/tests)
else ()
    message(STATUS "Tests are DISABLED (BUILD_TESTS=OFF)")
endif (BUILD_TESTS)

if (GENERATE_DOCS)
    message(STATUS "Api doc generation enabled (GENERATE_DOCS=ON)")
else ()
    message(STATUS "Api doc generation DISABLED (GENERATE_DOCS=OFF)")
endif (GENERATE_DOCS)
