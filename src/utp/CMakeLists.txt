set(utp_HDR
    utpserver.h
    utpserverthread.h
    utpprotocol.h
    utpsocket.h
    connection.h
    localwindow.h
    remotewindow.h
    timevalue.h
    pollpipe.h
    delaywindow.h
    packetbuffer.h
)

install(FILES ${utp_HDR} DESTINATION ${INCLUDE_INSTALL_DIR}/libktorrent/utp COMPONENT Devel)
add_subdirectory(tests)
