set (magnet_HDR
    magnetdownloader.h
    magnetlink.h
    metadatadownload.h
)

install(FILES ${magnet_HDR} DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF5}/libktorrent/magnet COMPONENT Devel)

add_subdirectory(tests)
