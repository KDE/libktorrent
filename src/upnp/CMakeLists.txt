set (upnp_HDR
    upnprouter.h
    upnpmcastsocket.h
)

install(FILES ${upnp_HDR} DESTINATION ${KDE_INSTALL_INCLUDEDIR}/libktorrent/upnp COMPONENT Devel)

if(HAVE_INSTALLED_LIBRARY_CHECK)
    ecm_installed_library_check_include_strings(KTorrent6 HEADERS ${upnp_HDR} PREFIX upnp)
endif()
