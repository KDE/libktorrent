if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

set(diskio_HDR
    cache.h
    cachefile.h
    singlefilecache.h
    preallocationthread.h
    preallocationjob.h
    movedatafilesjob.h
    deletedatafilesjob.h
    chunkmanager.h
    dndfile.h
    chunk.h
    multifilecache.h
    piecedata.h
)

install(FILES ${diskio_HDR} DESTINATION ${KDE_INSTALL_INCLUDEDIR}/libktorrent/diskio COMPONENT Devel)

if(HAVE_INSTALLED_LIBRARY_CHECK)
    ecm_installed_library_check_include_strings(KTorrent6 HEADERS ${diskio_HDR} PREFIX diskio)
endif()
