set(dht_HDR
    dhtbase.h
    rpcserver.h
    rpcserverinterface.h
    key.h
    pack.h
    rpcmsg.h
    dhtpeersource.h
    announcetask.h
    rpccall.h
    kclosestnodessearch.h
    dht.h
    node.h
    task.h
    nodelookup.h
    taskmanager.h
    kbucket.h
    kbucketentry.h
    kbuckettable.h
    database.h
    announcereq.h
    announcersp.h
    pingreq.h
    pingrsp.h
    findnodereq.h
    findnodersp.h
    getpeersreq.h
    getpeersrsp.h
    rpcmsgfactory.h
    packednodecontainer.h
)

install(FILES ${dht_HDR} DESTINATION ${KDE_INSTALL_INCLUDEDIR}/libktorrent/dht COMPONENT Devel)

if(HAVE_INSTALLED_LIBRARY_CHECK)
    ecm_installed_library_check_include_strings(KTorrent6 HEADERS ${dht_HDR} PREFIX dht)
endif()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
