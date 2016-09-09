add_subdirectory(tests)

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

install(FILES ${net_HDR} DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF5}/libktorrent/net COMPONENT Devel)
