set(peer_HDR
    peeruploader.h
    peer.h
    peermanager.h
    authenticatebase.h
    authenticate.h
    peerprotocolextension.h
    utpex.h
    utmetadata.h
    packetreader.h
    authenticationmonitor.h
    peerid.h
    peerdownloader.h
    chunkcounter.h
    serverauthenticate.h
    accessmanager.h
    peerconnector.h
    superseeder.h
    connectionlimit.h
)

install(FILES ${peer_HDR} DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF}/libktorrent/peer COMPONENT Devel)

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
