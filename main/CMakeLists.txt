include(FetchContent)
FetchContent_Declare(cli11_proj
        URL https://github.com/CLIUtils/CLI11/archive/refs/tags/v2.4.1.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP true
        EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(cli11_proj)

add_executable(krico-backup main.cpp)
target_link_libraries(krico-backup libKricoBackup CLI11::CLI11)

install(TARGETS krico-backup)