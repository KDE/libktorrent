set(tracker_HDR
    tracker.h
    udptracker.h
    udptrackersocket.h
    httptracker.h
    trackermanager.h
    httpannouncejob.h
    kioannouncejob.h
)

install(FILES ${tracker_HDR} DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF5}/libktorrent/tracker COMPONENT Devel)
