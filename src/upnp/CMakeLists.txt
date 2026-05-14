set (upnp_HDR
    upnprouter.h
    upnpmcastsocket.h
)

install(FILES ${upnp_HDR} DESTINATION ${KDE_INSTALL_INCLUDEDIR}/libktorrent/upnp COMPONENT Devel)
