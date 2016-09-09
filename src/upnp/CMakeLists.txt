set (upnp_HDR
    upnprouter.h
    upnpmcastsocket.h
)

install(FILES ${upnp_HDR} DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF5}/libktorrent/upnp COMPONENT Devel)
