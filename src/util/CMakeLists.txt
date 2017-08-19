if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

set(util_HDR
    autorotatelogjob.h
    fileops.h
    timer.h
    ptrmap.h
    log.h
    waitjob.h
    urlencoder.h
    array.h
    functions.h
    file.h
    constants.h
    bitset.h
    sha1hash.h
    sha1hashgen.h
    error.h
    win32.h
    logsystemmanager.h
    compressfilejob.h
    decompressfilejob.h
    extractfilejob.h
    pipe.h
    circularbuffer.h
    resourcemanager.h
    bufferpool.h
)
if(NOT WIN32)
set(util_HDR ${util_HDR}
    signalcatcher.h
    )
endif(NOT WIN32)

install(FILES ${util_HDR} DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF5}/libktorrent/util COMPONENT Devel)
