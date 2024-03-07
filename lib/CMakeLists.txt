add_library(libKricoBackup
        include/krico/backup/exception.h
        include/krico/backup/TemporaryFile.h
        include/krico/backup/TemporaryDirectory.h
        include/krico/backup/Directory.h
        include/krico/backup/settings.h

        src/TemporaryFile.cpp
        src/TemporaryDirectory.cpp
        src/Directory.cpp
        src/settings.cpp
        include/krico/backup/Digest.h
        src/Digest.cpp
)

target_include_directories(libKricoBackup PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

find_package(OpenSSL 3.2 REQUIRED)
message(STATUS "Found OpenSSL (version ${OPENSSL_VERSION})")
target_link_libraries(libKricoBackup OpenSSL::Crypto)

if (GENERATE_DOCS)
    find_package(Doxygen)

    if (Doxygen_FOUND)
        message(STATUS "Found Doxygen (version ${DOXYGEN_VERSION})")
        doxygen_add_docs(libKricoBackupApiDoc
                include
                COMMENT "Generate libKricoBackup doxygen api docs"
        )
    else (Doxygen_FOUND)
        message(WARNING "Could not find doxygen, api doc will not be generated.
        You can set GENERATE_DOCS=OFF to disable this message")
    endif (Doxygen_FOUND)
endif (GENERATE_DOCS)