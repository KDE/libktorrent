set(tracker_HDR
    tracker.h
    udptracker.h
    udptrackersocket.h
    httptracker.h
    trackermanager.h
    httpannouncejob.h
    kioannouncejob.h
)

install(FILES ${tracker_HDR} DESTINATION ${INCLUDE_INSTALL_DIR}/libktorrent/tracker COMPONENT Devel)
