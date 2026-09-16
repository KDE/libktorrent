set (magnet_HDR
    magnetdownloader.h
    magnetlink.h
    metadatadownload.h
)

install(FILES ${magnet_HDR} DESTINATION ${KDE_INSTALL_INCLUDEDIR}/libktorrent/magnet COMPONENT Devel)

if(HAVE_INSTALLED_LIBRARY_CHECK)
    ecm_installed_library_check_include_strings(KTorrent6 HEADERS ${magnet_HDR} PREFIX magnet)
endif()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
