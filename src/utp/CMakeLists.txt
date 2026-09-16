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

install(FILES ${utp_HDR} DESTINATION ${KDE_INSTALL_INCLUDEDIR}/libktorrent/utp COMPONENT Devel)

if(HAVE_INSTALLED_LIBRARY_CHECK)
    ecm_installed_library_check_include_strings(KTorrent6 HEADERS ${utp_HDR} PREFIX utp)
endif()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
