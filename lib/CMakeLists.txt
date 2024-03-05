add_library(libKricoBackup
        include/krico/backup/settings.h
        src/settings.cpp
)
target_include_directories(libKricoBackup PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)