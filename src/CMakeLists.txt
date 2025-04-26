configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config-ktorrent.h.cmake ${CMAKE_BINARY_DIR}/config-ktorrent.h)

remove_definitions(-DQT_NO_HTTP)

add_library(KTorrent6)
set_target_properties(KTorrent6 PROPERTIES
    VERSION     ${RELEASE_SERVICE_VERSION}
    SOVERSION   ${SOVERSION}
    EXPORT_NAME KTorrent6
)

target_sources(KTorrent6 PRIVATE
    util/bitset.cpp
    util/timer.cpp
    util/urlencoder.cpp
    util/sha1hashgen.cpp
    util/sha1hash.cpp
    util/functions.cpp
    util/ptrmap.cpp
    util/array.cpp
    util/error.cpp
    util/file.cpp
    util/fileops.cpp
    util/log.cpp
    util/waitjob.cpp
    util/autorotatelogjob.cpp
    util/compressfilejob.cpp
    util/decompressfilejob.cpp
    util/extractfilejob.cpp
    util/logsystemmanager.cpp
    util/pipe.cpp
    util/circularbuffer.cpp
    util/resourcemanager.cpp
    util/bufferpool.cpp

    bcodec/bdecoder.cpp
    bcodec/bencoder.cpp
    bcodec/bnode.cpp
    bcodec/value.cpp

    net/address.cpp
    net/packetsocket.cpp
    net/portlist.cpp
    net/socket.cpp
    net/socketmonitor.cpp
    net/speed.cpp
    net/uploadthread.cpp
    net/downloadthread.cpp
    net/networkthread.cpp
    net/socketgroup.cpp
    net/socks.cpp
    net/wakeuppipe.cpp
    net/reverseresolver.cpp
    net/socketdevice.cpp
    net/poll.cpp
    net/serversocket.cpp
    net/addressresolver.cpp
    net/trafficshapedsocket.cpp
    net/streamsocket.cpp

    mse/bigint.cpp
    mse/functions.cpp
    mse/rc4encryptor.cpp
    mse/encryptedpacketsocket.cpp
    mse/encryptedauthenticate.cpp
    mse/encryptedserverauthenticate.cpp

    peer/authenticatebase.cpp
    peer/authenticate.cpp
    peer/serverauthenticate.cpp
    peer/authenticationmonitor.cpp
    peer/chunkcounter.cpp
    peer/peerid.cpp
    peer/peer.cpp
    peer/peermanager.cpp
    peer/peerdownloader.cpp
    peer/peeruploader.cpp
    peer/packetreader.cpp
    peer/peerprotocolextension.cpp
    peer/utpex.cpp
    peer/utmetadata.cpp
    peer/accessmanager.cpp
    peer/badpeerslist.cpp
    peer/peerconnector.cpp
    peer/superseeder.cpp
    peer/connectionlimit.cpp

    #download/piece.cpp all the code is inlined
    #download/request.cpp all the code is inlined
    download/packet.cpp
    download/webseed.cpp
    download/chunkdownload.cpp
    download/chunkselector.cpp
    download/downloader.cpp
    download/httpconnection.cpp
    download/httpresponseheader.cpp
    download/streamingchunkselector.cpp

    interfaces/piecedownloader.cpp
    interfaces/peerinterface.cpp
    interfaces/peersource.cpp
    interfaces/torrentinterface.cpp
    interfaces/torrentfileinterface.cpp
    interfaces/trackerslist.cpp
    interfaces/logmonitorinterface.cpp
    interfaces/blocklistinterface.cpp
    interfaces/exitoperation.cpp
    interfaces/chunkdownloadinterface.cpp
    interfaces/monitorinterface.cpp
    interfaces/queuemanagerinterface.cpp
    interfaces/chunkselectorinterface.cpp
    interfaces/cachefactory.cpp
    interfaces/webseedinterface.cpp
    interfaces/trackerinterface.cpp
    interfaces/serverinterface.cpp

    torrent/globals.cpp
    torrent/server.cpp
    torrent/torrent.cpp
    torrent/torrentfile.cpp
    torrent/peersourcemanager.cpp
    torrent/uploader.cpp
    torrent/timeestimator.cpp
    torrent/statsfile.cpp
    torrent/choker.cpp
    torrent/advancedchokealgorithm.cpp
    torrent/torrentcontrol.cpp
    torrent/torrentcreator.cpp
    torrent/torrentstats.cpp
    torrent/jobqueue.cpp
    torrent/job.cpp
    torrent/torrentfilestream.cpp

    dht/announcetask.cpp
    dht/dht.cpp
    dht/kclosestnodessearch.cpp
    dht/nodelookup.cpp
    dht/rpcmsg.cpp
    dht/rpcmsgfactory.cpp
    dht/taskmanager.cpp
    dht/database.cpp
    dht/dhtpeersource.cpp
    dht/key.cpp
    dht/pack.cpp
    dht/rpcserver.cpp
    dht/rpcserverinterface.cpp
    dht/dhtbase.cpp
    dht/kbucket.cpp
    dht/kbucketentry.cpp
    dht/kbuckettable.cpp
    dht/node.cpp
    dht/rpccall.cpp
    dht/task.cpp
    dht/packednodecontainer.cpp
    dht/errmsg.cpp
    dht/pingreq.cpp
    dht/findnodereq.cpp
    dht/getpeersreq.cpp
    dht/announcereq.cpp
    dht/pingrsp.cpp
    dht/findnodersp.cpp
    dht/getpeersrsp.cpp
    dht/announcersp.cpp

    diskio/cache.cpp
    diskio/chunk.cpp
    diskio/dndfile.cpp
    diskio/singlefilecache.cpp
    diskio/multifilecache.cpp
    diskio/preallocationthread.cpp
    diskio/preallocationjob.cpp
    diskio/movedatafilesjob.cpp
    diskio/deletedatafilesjob.cpp
    diskio/piecedata.cpp
    diskio/cachefile.cpp
    diskio/chunkmanager.cpp

    tracker/httptracker.cpp
    tracker/tracker.cpp
    tracker/udptracker.cpp
    tracker/udptrackersocket.cpp
    tracker/trackermanager.cpp
    #tracker/httpannouncejob.cpp
    tracker/kioannouncejob.cpp

    datachecker/datachecker.cpp
    datachecker/datacheckerthread.cpp
    datachecker/singledatachecker.cpp
    datachecker/multidatachecker.cpp
    datachecker/datacheckerjob.cpp

    magnet/magnetdownloader.cpp
    magnet/magnetlink.cpp
    magnet/metadatadownload.cpp

    utp/utpserver.cpp
    utp/utpserverthread.cpp
    utp/connection.cpp
    utp/localwindow.cpp
    utp/remotewindow.cpp
    utp/utpsocket.cpp
    utp/timevalue.cpp
    utp/utpprotocol.cpp
    utp/pollpipe.cpp
    utp/delaywindow.cpp
    utp/outputqueue.cpp
    utp/packetbuffer.cpp

    upnp/soap.cpp
    upnp/upnpmcastsocket.cpp
    upnp/upnprouter.cpp
    upnp/upnpdescriptionparser.cpp
    upnp/httprequest.cpp

    version.cpp
)

if(NOT WIN32)
# Add signalcatcher.cpp for all builds except WIN32
    target_sources(KTorrent6 PRIVATE util/signalcatcher.cpp)
endif(NOT WIN32)

if(WIN32)
# Compile stuff for windows
    target_sources(KTorrent6 PRIVATE util/win32.cpp)
endif(WIN32)


# ki18n_wrap_ui(KTorrent6 ${libktorrent_UI})
# kconfig_add_kcfg_files(KTorrent6 settings.kcfgc)

generate_export_header(KTorrent6 BASE_NAME ktorrent)

target_link_libraries(KTorrent6
PUBLIC
    KF6::Archive
    KF6::KIOCore
    KF6::KIOWidgets
    KF6::Solid
    KF6::I18n
    Qt6::Xml
    Boost::boost
    LibGMP::LibGMP
)

if (USE_OPENSSL)
    target_compile_definitions(KTorrent6 PUBLIC LIBKTORRENT_USE_OPENSSL=1)
    target_link_libraries(KTorrent6 PUBLIC OpenSSL::Crypto)
elseif (USE_LIBGCRYPT)
    target_compile_definitions(KTorrent6 PUBLIC LIBKTORRENT_USE_LIBGCRYPT=1)
    target_link_libraries(KTorrent6 PUBLIC LibGcrypt::LibGcrypt)
endif()

if(WIN32)
target_link_libraries(KTorrent6 PUBLIC ws2_32)
endif(WIN32)
if(HAIKU)
target_link_libraries(KTorrent6 PUBLIC network)
endif(HAIKU)
target_include_directories(KTorrent6 INTERFACE "$<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR_KF}/libktorrent>")
target_include_directories(KTorrent6 PUBLIC "$<BUILD_INTERFACE:${LIBKTORRENT_SOURCE_DIR}/src;${LIBKTORRENT_BINARY_DIR}/src;${LIBKTORRENT_BINARY_DIR}>")


install(TARGETS KTorrent6 EXPORT KTorrent6Targets ${KF_INSTALL_TARGETS_DEFAULT_ARGS})

set(libktorrent_export_HEADERS
    ${CMAKE_CURRENT_BINARY_DIR}/ktorrent_export.h
    ${CMAKE_BINARY_DIR}/libktorrent_version.h
    version.h
)

set(libktorrent_HEADERS
    bcodec/bencoder.h
    bcodec/bnode.h
    bcodec/bdecoder.h
    bcodec/value.h

    datachecker/datachecker.h
    datachecker/datacheckerjob.h
    datachecker/datacheckerthread.h
    datachecker/multidatachecker.h
    datachecker/singledatachecker.h

    dht/announcereq.h
    dht/announcersp.h
    dht/announcetask.h
    dht/database.h
    dht/dht.h
    dht/dhtbase.h
    dht/dhtpeersource.h
    dht/findnodereq.h
    dht/findnodersp.h
    dht/getpeersreq.h
    dht/getpeersrsp.h
    dht/kbucket.h
    dht/kbucketentry.h
    dht/kbuckettable.h
    dht/kclosestnodessearch.h
    dht/key.h
    dht/node.h
    dht/nodelookup.h
    dht/pack.h
    dht/pingreq.h
    dht/pingrsp.h
    dht/rpccall.h
    dht/rpcmsg.h
    dht/rpcmsgfactory.h
    dht/rpcserver.h
    dht/rpcserverinterface.h
    dht/task.h
    dht/taskmanager.h

    diskio/cache.h
    diskio/cachefile.h
    diskio/chunk.h
    diskio/chunkmanager.h
    diskio/deletedatafilesjob.h
    diskio/dndfile.h
    diskio/movedatafilesjob.h
    diskio/multifilecache.h
    diskio/piecedata.h
    diskio/preallocationjob.h
    diskio/preallocationthread.h
    diskio/singlefilecache.h

    download/chunkdownload.h
    download/chunkselector.h
    download/downloader.h
    download/packet.h
    download/piece.h
    download/request.h
    download/webseed.h

    interfaces/peerinterface.h
    interfaces/queuemanagerinterface.h
    interfaces/exitoperation.h
    interfaces/chunkdownloadinterface.h
    interfaces/trackerslist.h
    interfaces/torrentinterface.h
    interfaces/logmonitorinterface.h
    interfaces/piecedownloader.h
    interfaces/monitorinterface.h
    interfaces/blocklistinterface.h
    interfaces/torrentfileinterface.h
    interfaces/peersource.h
    interfaces/chunkselectorinterface.h
    interfaces/cachefactory.h
    interfaces/webseedinterface.h
    interfaces/trackerinterface.h
    interfaces/serverinterface.h

    magnet/magnetdownloader.h
    magnet/magnetlink.h
    magnet/metadatadownload.h

    mse/bigint.h
    mse/encryptedauthenticate.h
    mse/encryptedpacketsocket.h
    mse/encryptedserverauthenticate.h
    mse/functions.h
    mse/rc4encryptor.h

    net/address.h
    net/addressresolver.h
    net/downloadthread.h
    net/networkthread.h
    net/packetsocket.h
    net/poll.h
    net/portlist.h
    net/reverseresolver.h
    net/serversocket.h
    net/socket.h
    net/socketdevice.h
    net/socketgroup.h
    net/socketmonitor.h
    net/socks.h
    net/speed.h
    net/streamsocket.h
    net/trafficshapedsocket.h
    net/uploadthread.h
    net/wakeuppipe.h

    peer/accessmanager.h
    peer/authenticate.h
    peer/authenticatebase.h
    peer/authenticationmonitor.h
    peer/chunkcounter.h
    peer/connectionlimit.h
    peer/packetreader.h
    peer/peer.h
    peer/peerconnector.h
    peer/peerdownloader.h
    peer/peerid.h
    peer/peermanager.h
    peer/peerprotocolextension.h
    peer/peeruploader.h
    peer/serverauthenticate.h
    peer/superseeder.h
    peer/utmetadata.h
    peer/utpex.h

    torrent/advancedchokealgorithm.h
    torrent/choker.h
    torrent/globals.h
    torrent/job.h
    torrent/jobqueue.h
    torrent/peersourcemanager.h
    torrent/server.h
    torrent/statsfile.h
    torrent/timeestimator.h
    torrent/torrent.h
    torrent/torrentcontrol.h
    torrent/torrentcreator.h
    torrent/torrentfile.h
    torrent/torrentfilestream.h
    torrent/torrentstats.h
    torrent/uploader.h

    tracker/httptracker.h
    tracker/kioannouncejob.h
    tracker/tracker.h
    tracker/trackermanager.h
    tracker/udptracker.h
    tracker/udptrackersocket.h

    upnp/upnpmcastsocket.h
    upnp/upnprouter.h

    util/array.h
    util/autorotatelogjob.h
    util/bitset.h
    util/bufferpool.h
    util/circularbuffer.h
    util/compressfilejob.h
    util/constants.h
    util/decompressfilejob.h
    util/error.h
    util/extractfilejob.h
    util/file.h
    util/fileops.h
    util/functions.h
    util/log.h
    util/logsystemmanager.h
    util/pipe.h
    util/ptrmap.h
    util/resourcemanager.h
    util/sha1hash.h
    util/sha1hashgen.h
    util/timer.h
    util/urlencoder.h
    util/waitjob.h
    util/win32.h

    utp/connection.h
    utp/delaywindow.h
    utp/localwindow.h
    utp/packetbuffer.h
    utp/pollpipe.h
    utp/remotewindow.h
    utp/timevalue.h
    utp/utpprotocol.h
    utp/utpserver.h
    utp/utpserverthread.h
    utp/utpsocket.h
)

if(NOT WIN32)
set(libktorrent_HEADERS ${libktorrent_HEADERS}
    util/signalcatcher.h
)
endif()

install(FILES
    ${libktorrent_HEADERS}
    ${libktorrent_export_HEADERS}
    DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF}/libktorrent COMPONENT Devel
)

if(BUILD_TESTING)
    add_subdirectory(datachecker/tests)
    add_subdirectory(dht/tests)
    add_subdirectory(diskio/tests)
    add_subdirectory(download/tests)
    add_subdirectory(magnet/tests)
    add_subdirectory(mse/tests)
    add_subdirectory(net/tests)
    add_subdirectory(peer/tests)
    add_subdirectory(torrent/tests)
    add_subdirectory(util/tests)
    add_subdirectory(utp/tests)
endif()

if (BUILD_QCH)
    ecm_add_qch(
        KTorrent6_QCH
        NAME libktorrent
        BASE_NAME libktorrent
        VERSION ${RELEASE_SERVICE_VERSION}
        ORG_DOMAIN org.kde
        SOURCES # using only public headers, to cover only public API
            ${libktorrent_HEADERS}
        MD_MAINPAGE "${CMAKE_SOURCE_DIR}/doc/index.md"
        LINK_QCHS
            Qt6Core_QCH
            Qt6Network_QCH
            Qt6Xml_QCH
            KF6Archive_QCH
            KF6KIO_QCH
        INCLUDE_DIRS
            ${CMAKE_CURRENT_BINARY_DIR}
        TAGFILE_INSTALL_DESTINATION ${KDE_INSTALL_QTQCHDIR}
        QCH_INSTALL_DESTINATION ${KDE_INSTALL_QTQCHDIR}
        COMPONENT Devel
    )
endif()
