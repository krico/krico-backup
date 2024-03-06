add_library(libKricoBackup
        include/krico/backup/exception.h
        include/krico/backup/TemporaryFile.h
        include/krico/backup/TemporaryDirectory.h
        include/krico/backup/settings.h

        src/TemporaryFile.cpp
        src/TemporaryDirectory.cpp
        src/settings.cpp
)
target_include_directories(libKricoBackup PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
