set (magnet_HDR
    magnetdownloader.h
    magnetlink.h
    metadatadownload.h
)

install(FILES ${magnet_HDR} DESTINATION ${KDE_INSTALL_INCLUDEDIR}/libktorrent/magnet COMPONENT Devel)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
