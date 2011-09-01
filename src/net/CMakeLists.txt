
add_subdirectory(tests)

set(net_HDR  
	socks.h
	bufferedsocket.h
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

install(FILES ${net_HDR} DESTINATION ${INCLUDE_INSTALL_DIR}/libktorrent/net COMPONENT Devel)

