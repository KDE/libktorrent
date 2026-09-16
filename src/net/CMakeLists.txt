if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

set(net_HDR
    socks.h
    packetsocket.h
    trafficshapedsocket.h
    streamsocket.h
    downloadthread.h
    address.h
    addressresolver.h
    socketgroup.h
    portlist.h
    networkthread.h
    socket.h
    speed.h
    uploadthread.h
    socketmonitor.h
    reverseresolver.h
    socketdevice.h
    poll.h
    wakeuppipe.h
    serversocket.h
)

install(FILES ${net_HDR} DESTINATION ${KDE_INSTALL_INCLUDEDIR}/libktorrent/net COMPONENT Devel)

if(HAVE_INSTALLED_LIBRARY_CHECK)
    ecm_installed_library_check_include_strings(KTorrent6 HEADERS ${net_HDR} PREFIX net)
endif()
