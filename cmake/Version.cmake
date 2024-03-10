set(KRICO_VERSION_INCLUDE "${CMAKE_CURRENT_BINARY_DIR}/version-include")
string(TIMESTAMP KRICO_VERSION_TS UTC)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.h.in ${KRICO_VERSION_INCLUDE}/krico/backup/version.h)

function(add_krico_version target)
    #    add_dependencies(${target} krico-version)
    target_include_directories(${target} PUBLIC ${KRICO_VERSION_INCLUDE})
    if (GENERATE_DOCS)
        find_package(Doxygen)
        if (Doxygen_FOUND)
            doxygen_add_docs(${target}VersionApiDoc
                    ${KRICO_VERSION_INCLUDE}
                    WORKING_DIRECTORY ${KRICO_VERSION_INCLUDE}
            )
            add_dependencies(apidoc ${target}VersionApiDoc)
        endif (Doxygen_FOUND)
    endif (GENERATE_DOCS)
endfunction()
