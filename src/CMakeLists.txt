configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config-ktorrent.h.cmake ${CMAKE_BINARY_DIR}/config-ktorrent.h)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${LIBGCRYPT_INCLUDE_DIR} ${QCA-qt5_INCLUDE_DIR})
remove_definitions(-DQT_NO_HTTP)

set(libktorrent_SRC
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
	
	migrate/cachemigrate.cpp  
	migrate/ccmigrate.cpp  
	migrate/migrate.cpp
	
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
set(libktorrent_SRC ${libktorrent_SRC} util/signalcatcher.cpp)
endif(NOT WIN32)

if(WIN32)
# Compile stuff for windows
set(libktorrent_SRC ${libktorrent_SRC} util/win32.cpp)
endif(WIN32)


# ki18n_wrap_ui(libktorrent_SRC ${libktorrent_UI})
# kconfig_add_kcfg_files(libktorrent_SRC settings.kcfgc)
add_library(ktorrent SHARED ${libktorrent_SRC})
generate_export_header(ktorrent BASE_NAME ktorrent)

target_link_libraries(ktorrent
	KF5::Archive
	KF5::Crash
	KF5::KIOCore
	KF5::KIOWidgets
	KF5::Solid
	KF5::I18n
	Qt5::Xml
	qca-qt5
	${GMP_LIBRARIES}
	${LIBGCRYPT_LIBRARIES}
)

if(WIN32)
target_link_libraries(ktorrent ws2_32)
endif(WIN32)
set_target_properties(ktorrent PROPERTIES VERSION 6.0.0 SOVERSION 6 )
install(TARGETS ktorrent EXPORT LibKTorrentTargets  ${INSTALL_TARGETS_DEFAULT_ARGS} )


set (interfaces_HDR
	./interfaces/peerinterface.h
	./interfaces/queuemanagerinterface.h
	./interfaces/exitoperation.h
	./interfaces/chunkdownloadinterface.h
	./interfaces/trackerslist.h
	./interfaces/torrentinterface.h
	./interfaces/logmonitorinterface.h
	./interfaces/piecedownloader.h
	./interfaces/monitorinterface.h
	./interfaces/blocklistinterface.h
	./interfaces/torrentfileinterface.h
	./interfaces/peersource.h
	./interfaces/chunkselectorinterface.h
	./interfaces/cachefactory.h
	./interfaces/webseedinterface.h
	./interfaces/trackerinterface.h
	./interfaces/serverinterface.h
)

set (ktorrent_HDR 
	${CMAKE_CURRENT_BINARY_DIR}/ktorrent_export.h
	version.h
)
	
set (bcodec_HDR
	./bcodec/bencoder.h
	./bcodec/bnode.h
	./bcodec/bdecoder.h
	./bcodec/value.h
)

set (migrate_HDR
	./migrate/ccmigrate.h
	./migrate/migrate.h
	./migrate/cachemigrate.h
)



install(FILES ${bcodec_HDR} DESTINATION ${INCLUDE_INSTALL_DIR}/libktorrent/bcodec COMPONENT Devel)
install(FILES ${interfaces_HDR} DESTINATION ${INCLUDE_INSTALL_DIR}/libktorrent/interfaces COMPONENT Devel)
install(FILES ${migrate_HDR} DESTINATION ${INCLUDE_INSTALL_DIR}/libktorrent/migrate COMPONENT Devel)
install(FILES ${ktorrent_HDR} DESTINATION ${INCLUDE_INSTALL_DIR}/libktorrent COMPONENT Devel)

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
