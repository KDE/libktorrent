set (upnp_HDR
    upnprouter.h
    upnpmcastsocket.h
)

install(FILES ${upnp_HDR} DESTINATION ${INCLUDE_INSTALL_DIR}/libktorrent/upnp COMPONENT Devel)
