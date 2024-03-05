include(FetchContent)
FetchContent_Declare(googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP true
        EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(googletest)

enable_testing()

include(GoogleTest)

add_executable(tests
        settings_test.cpp
)
target_link_libraries(tests libKricoBackup GTest::gtest_main)
gtest_discover_tests(tests)