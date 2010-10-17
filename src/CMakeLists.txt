configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config-ktorrent.h.cmake ${CMAKE_BINARY_DIR}/config-ktorrent.h)

include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${QCA2_INCLUDE_DIR})
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
	
	bcodec/bdecoder.cpp  
	bcodec/bencoder.cpp  
	bcodec/bnode.cpp  
	bcodec/value.cpp

	net/address.cpp  
	net/bufferedsocket.cpp  
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
	
	mse/bigint.cpp  
	mse/functions.cpp  
	mse/rc4encryptor.cpp  
	mse/streamsocket.cpp
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
	peer/packetwriter.cpp
	peer/packetreader.cpp
	peer/peerprotocolextension.cpp
	peer/utpex.cpp
	peer/utmetadata.cpp
	peer/accessmanager.cpp
	peer/badpeerslist.cpp
	peer/peerconnector.cpp

	download/piece.cpp
	download/request.cpp
	download/packet.cpp
	download/webseed.cpp
	download/chunkdownload.cpp
	download/chunkselector.cpp
	download/downloader.cpp
	download/httpconnection.cpp

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

	dht/announcetask.cpp  
	dht/dht.cpp                
	dht/kclosestnodessearch.cpp  
	dht/nodelookup.cpp  
	dht/rpcmsg.cpp     
	dht/taskmanager.cpp
	dht/database.cpp      
	dht/dhtpeersource.cpp 
	dht/key.cpp                  
	dht/pack.cpp        
	dht/rpcserver.cpp
	dht/dhtbase.cpp       
	dht/kbucket.cpp            
	dht/node.cpp                 
	dht/rpccall.cpp     
	dht/task.cpp

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
	tracker/httpannouncejob.cpp
	tracker/kioannouncejob.cpp
	
	datachecker/datachecker.cpp          
	datachecker/datacheckerthread.cpp  
	datachecker/singledatachecker.cpp
	datachecker/datacheckerlistener.cpp  
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
	
	version.cpp
)

if(WIN32)
# Compile stuff for windows
set(libktorrent_SRC ${libktorrent_SRC} util/win32.cpp)
endif(WIN32)


# kde4_add_ui_files(libktorrent_SRC ${libktorrent_UI})
# kde4_add_kcfg_files(libktorrent_SRC settings.kcfgc)
kde4_add_library(ktorrent SHARED ${libktorrent_SRC})

target_link_libraries(ktorrent ${KDE4_KIO_LIBS} ${GMP_LIBRARIES} ${QCA2_LIBRARIES})
if(WIN32)
target_link_libraries(ktorrent ws2_32)
endif(WIN32)
set_target_properties(ktorrent PROPERTIES VERSION 2.0.3 SOVERSION 2)
install(TARGETS ktorrent  ${INSTALL_TARGETS_DEFAULT_ARGS} )


set (mse_HDR 
	./mse/encryptedserverauthenticate.h
	./mse/functions.h
	./mse/rc4encryptor.h
	./mse/encryptedauthenticate.h
	./mse/bigint.h
	./mse/streamsocket.h
)

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

set (peer_HDR
	./peer/peeruploader.h
	./peer/packetwriter.h
	./peer/peer.h
	./peer/peermanager.h
	./peer/authenticatebase.h
	./peer/authenticate.h
	./peer/peerprotocolextension.h
	./peer/utpex.h
	./peer/utmetadata.h
	./peer/packetreader.h
	./peer/authenticationmonitor.h
	./peer/peerid.h
	./peer/peerdownloader.h
	./peer/chunkcounter.h
	./peer/serverauthenticate.h
	./peer/accessmanager.h
	./peer/peerconnector.h
)
	

	
set (download_HDR
	./download/chunkdownload.h
	./download/downloader.h
	./download/request.h
	./download/piece.h
	./download/packet.h
	./download/chunkselector.h
	./download/webseed.h
)

set (ktorrent_HDR 
	ktorrent_export.h
	version.h
)
	
set (bcodec_HDR
	./bcodec/bencoder.h
	./bcodec/bnode.h
	./bcodec/bdecoder.h
	./bcodec/value.h
)
	
set (diskio_HDR  
	./diskio/cache.h
	./diskio/cachefile.h
	./diskio/singlefilecache.h
	./diskio/preallocationthread.h
	./diskio/preallocationjob.h
	./diskio/movedatafilesjob.h
	./diskio/deletedatafilesjob.h
	./diskio/chunkmanager.h
	./diskio/dndfile.h
	./diskio/chunk.h
	./diskio/multifilecache.h
	./diskio/piecedata.h
)
	

	
set (tracker_HDR
	./tracker/tracker.h
	./tracker/udptracker.h
	./tracker/udptrackersocket.h
	./tracker/httptracker.h
	./tracker/trackermanager.h
	./tracker/httpannouncejob.h
	./tracker/kioannouncejob.h
)

set (datachecker_HDR
	./datachecker/datacheckerthread.h
	./datachecker/datachecker.h
	./datachecker/singledatachecker.h
	./datachecker/datacheckerlistener.h
	./datachecker/multidatachecker.h
	./datachecker/datacheckerjob.h
)

set (dht_HDR
	./dht/dhtbase.h
	./dht/rpcserver.h
	./dht/key.h
	./dht/pack.h
	./dht/rpcmsg.h
	./dht/dhtpeersource.h
	./dht/announcetask.h
	./dht/rpccall.h
	./dht/kclosestnodessearch.h
	./dht/dht.h
	./dht/node.h
	./dht/task.h
	./dht/nodelookup.h
	./dht/taskmanager.h
	./dht/kbucket.h
	./dht/database.h
)

set (migrate_HDR
	./migrate/ccmigrate.h
	./migrate/migrate.h
	./migrate/cachemigrate.h
)



install(FILES ${tracker_HDR} DESTINATION ${INCLUDE_INSTALL_DIR}/libktorrent/tracker COMPONENT Devel)
install(FILES ${diskio_HDR} DESTINATION ${INCLUDE_INSTALL_DIR}/libktorrent/diskio COMPONENT Devel)
install(FILES ${bcodec_HDR} DESTINATION ${INCLUDE_INSTALL_DIR}/libktorrent/bcodec COMPONENT Devel)
install(FILES ${mse_HDR} DESTINATION ${INCLUDE_INSTALL_DIR}/libktorrent/mse COMPONENT Devel)
install(FILES ${interfaces_HDR} DESTINATION ${INCLUDE_INSTALL_DIR}/libktorrent/interfaces COMPONENT Devel)
install(FILES ${peer_HDR} DESTINATION ${INCLUDE_INSTALL_DIR}/libktorrent/peer COMPONENT Devel)
install(FILES ${datachecker_HDR} DESTINATION ${INCLUDE_INSTALL_DIR}/libktorrent/datachecker COMPONENT Devel)
install(FILES ${dht_HDR} DESTINATION ${INCLUDE_INSTALL_DIR}/libktorrent/dht COMPONENT Devel)
install(FILES ${migrate_HDR} DESTINATION ${INCLUDE_INSTALL_DIR}/libktorrent/migrate COMPONENT Devel)
install(FILES ${download_HDR} DESTINATION ${INCLUDE_INSTALL_DIR}/libktorrent/download COMPONENT Devel)
install(FILES ${ktorrent_HDR} DESTINATION ${INCLUDE_INSTALL_DIR}/libktorrent COMPONENT Devel)

add_subdirectory(torrent)
add_subdirectory(net)
add_subdirectory(magnet)
add_subdirectory(util)
add_subdirectory(utp)
