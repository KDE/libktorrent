set(tracker_HDR
    tracker.h
    udptracker.h
    udptrackersocket.h
    httptracker.h
    trackermanager.h
    kioannouncejob.h
)

install(FILES ${tracker_HDR} DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF}/libktorrent/tracker COMPONENT Devel)
