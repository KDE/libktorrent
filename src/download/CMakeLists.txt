if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

set (download_HDR
    chunkdownload.h
    downloader.h
    request.h
    piece.h
    packet.h
    chunkselector.h
    webseed.h
)

install(FILES ${download_HDR} DESTINATION ${KDE_INSTALL_INCLUDEDIR}/libktorrent/download COMPONENT Devel)

if(HAVE_INSTALLED_LIBRARY_CHECK)
    ecm_installed_library_check_include_strings(KTorrent6 HEADERS ${download_HDR} PREFIX download)
endif()
