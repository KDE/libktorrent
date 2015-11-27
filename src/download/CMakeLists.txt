add_subdirectory(tests)

set (download_HDR
	chunkdownload.h
	downloader.h
	request.h
	piece.h
	packet.h
	chunkselector.h
	webseed.h
)

install(FILES ${download_HDR} DESTINATION ${INCLUDE_INSTALL_DIR}/libktorrent/download COMPONENT Devel)