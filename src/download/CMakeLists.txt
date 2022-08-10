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

install(FILES ${download_HDR} DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF}/libktorrent/download COMPONENT Devel)
