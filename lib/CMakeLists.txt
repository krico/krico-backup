include(FetchContent)
FetchContent_Declare(spdlog_proj
        URL https://github.com/gabime/spdlog/archive/refs/tags/v1.13.0.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP true
        EXCLUDE_FROM_ALL
)
FetchContent_MakeAvailable(spdlog_proj)

add_library(libKricoBackup
        include/gtest/gtest_prod.h

        include/krico/backup/exception.h
        include/krico/backup/TemporaryFile.h
        include/krico/backup/TemporaryDirectory.h
        include/krico/backup/Directory.h
        include/krico/backup/Digest.h
        include/krico/backup/io.h
        include/krico/backup/uint8_utils.h
        include/krico/backup/FileLock.h
        include/krico/backup/BackupConfig.h
        include/krico/backup/BackupRepository.h
        include/krico/backup/BackupDirectoryId.h
        include/krico/backup/BackupDirectory.h
        include/krico/backup/BackupRunner.h
        include/krico/backup/BackupRepositoryLog.h
        include/krico/backup/BackupSummary.h

        src/TemporaryFile.cpp
        src/TemporaryDirectory.cpp
        src/Directory.cpp
        src/Digest.cpp
        src/io.cpp
        src/FileLock.cpp
        src/BackupConfig.cpp
        src/BackupRepository.cpp
        src/BackupDirectoryId.cpp
        src/BackupDirectory.cpp
        src/BackupRunner.cpp
        src/BackupRepositoryLog.cpp
        src/BackupSummary.cpp
        include/krico/backup/os.h
        src/os.cpp
)

target_include_directories(libKricoBackup PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_krico_version(libKricoBackup)

find_package(OpenSSL 3.2 REQUIRED)
message(STATUS "Found OpenSSL (version ${OPENSSL_VERSION})")
target_link_libraries(libKricoBackup OpenSSL::Crypto)

# spdlog
target_link_libraries(libKricoBackup spdlog::spdlog)

if (GENERATE_DOCS)
    find_package(Doxygen)

    if (Doxygen_FOUND)
        message(STATUS "Found Doxygen (version ${DOXYGEN_VERSION})")
        doxygen_add_docs(libKricoBackupApiDoc
                ${CMAKE_CURRENT_SOURCE_DIR}/include
                WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include
                COMMENT "Generate libKricoBackup doxygen api docs"
        )
        add_dependencies(apidoc libKricoBackupApiDoc)
    else (Doxygen_FOUND)
        message(WARNING "Could not find doxygen, api doc will not be generated.
        You can set GENERATE_DOCS=OFF to disable this message")
    endif (Doxygen_FOUND)
endif (GENERATE_DOCS)