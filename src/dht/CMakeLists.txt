set (dht_HDR
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
)

install(FILES ${dht_HDR} DESTINATION ${INCLUDE_INSTALL_DIR}/libktorrent/dht COMPONENT Devel)

#add_subdirectory(tests)