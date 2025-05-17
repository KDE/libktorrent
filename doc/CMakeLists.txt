find_package(Doxygen)
if (DOXYGEN_FOUND AND DOXYGEN_DOT_FOUND)
    message (STATUS "Doxygen and dot found. make docs target is available")
    # Add support for a docs target

    set(TAGFILES "${CMAKE_SOURCE_DIR}/scripts/qttags.tag=http://qt.nokia.com/doc/${QT_VERSION_MAJOR}.${QT_VERSION_MINOR}")
    set(DOCS_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/apidocs")
    configure_file("Doxyfile.in" "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile" IMMEDIATE @ONLY)

    # Add support for a Qt assistant docs target
    # qttags.tag is too big to distribute with Grantlee. Get it
    # from http://api.kde.org/kdereview-api/kdereview.tar.gz for example.
    if(EXISTS ${CMAKE_SOURCE_DIR}/scripts/qttags.tag)
        set(DOCS_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/qtdocs")
        set(TAGFILES "${CMAKE_SOURCE_DIR}/scripts/qttags.tag=qthelp://com.trolltech.qt/qdoc/")
        set(GENERATE_QHP "yes")

        configure_file("${CMAKE_MODULE_PATH}/Doxyfile.in" "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile.qt" IMMEDIATE @ONLY)
    endif()

    set( DOCS_ARGS ${DOXYGEN_EXECUTABLE} "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile")
    if(EXISTS "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile.qt")
        set(DOCS_ARGS ${DOCS_ARGS} COMMAND ${DOXYGEN_EXECUTABLE} "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile.qt")
    endif()

    add_custom_target(docs ${DOCS_ARGS})
endif()
