cmake_minimum_required(VERSION 3.22)
project(krico-backup VERSION 1.0 DESCRIPTION "Space-efficient backups for personal use" LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_TESTS "Enable building and running of unit tests" ON)

add_subdirectory(lib)
add_subdirectory(main)

if (BUILD_TESTS)
    message(STATUS "Tests are enabled")
    add_subdirectory(lib/tests)
else ()
    message(STATUS "Tests are DISABLED")
endif ()
