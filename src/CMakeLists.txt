configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config-ktorrent.h.cmake ${CMAKE_BINARY_DIR}/config-ktorrent.h)

remove_definitions(-DQT_NO_HTTP)

add_library(KF5Torrent)
add_library(KF5::Torrent ALIAS KF5Torrent)
set_target_properties(KF5Torrent PROPERTIES
    VERSION     ${RELEASE_SERVICE_VERSION}
    SOVERSION   ${SOVERSION}
    EXPORT_NAME Torrent
)

target_sources(KF5Torrent PRIVATE
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
    target_sources(KF5Torrent PRIVATE util/signalcatcher.cpp)
endif(NOT WIN32)

if(WIN32)
# Compile stuff for windows
    target_sources(KF5Torrent PRIVATE util/win32.cpp)
endif(WIN32)


# ki18n_wrap_ui(KF5Torrent ${libktorrent_UI})
# kconfig_add_kcfg_files(KF5Torrent settings.kcfgc)

generate_export_header(KF5Torrent BASE_NAME ktorrent)

target_link_libraries(KF5Torrent
PRIVATE
    qca-qt${QT_MAJOR_VERSION}
PUBLIC
    KF5::Archive
    KF5::KIOCore
    KF5::KIOWidgets
    KF5::Solid
    KF5::I18n
    Qt::Xml
    Boost::boost
    LibGcrypt::LibGcrypt
    LibGMP::LibGMP
)

if (QT_MAJOR_VERSION EQUAL "6")
    target_link_libraries(KF5Torrent PUBLIC Qt6::Core5Compat)
endif()

if(WIN32)
target_link_libraries(KF5Torrent ws2_32)
endif(WIN32)
target_include_directories(KF5Torrent INTERFACE "$<INSTALL_INTERFACE:${KDE_INSTALL_INCLUDEDIR_KF}/libktorrent>")
target_include_directories(KF5Torrent PUBLIC "$<BUILD_INTERFACE:${LIBKTORRENT_SOURCE_DIR}/src;${LIBKTORRENT_BINARY_DIR}/src;${LIBKTORRENT_BINARY_DIR}>")


install(TARGETS KF5Torrent EXPORT KF5TorrentTargets ${KF_INSTALL_TARGETS_DEFAULT_ARGS})

set (interfaces_HDR
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
)

set (ktorrent_HDR
    ${CMAKE_CURRENT_BINARY_DIR}/ktorrent_export.h
    ${CMAKE_BINARY_DIR}/libktorrent_version.h
    version.h
)

set (bcodec_HDR
    bcodec/bencoder.h
    bcodec/bnode.h
    bcodec/bdecoder.h
    bcodec/value.h
)

install(FILES ${bcodec_HDR} DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF}/libktorrent/bcodec COMPONENT Devel)
install(FILES ${interfaces_HDR} DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF}/libktorrent/interfaces COMPONENT Devel)
install(FILES ${migrate_HDR} DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF}/libktorrent/migrate COMPONENT Devel)
install(FILES ${ktorrent_HDR} DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF}/libktorrent COMPONENT Devel)

add_subdirectory(torrent)
add_subdirectory(datachecker)
add_subdirectory(download)
add_subdirectory(diskio)
add_subdirectory(peer)
add_subdirectory(net)
add_subdirectory(mse)
add_subdirectory(magnet)
add_subdirectory(util)
add_subdirectory(utp)
add_subdirectory(upnp)
add_subdirectory(tracker)
add_subdirectory(dht)
