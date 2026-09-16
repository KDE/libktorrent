set(tracker_HDR
    tracker.h
    udptracker.h
    udptrackersocket.h
    httptracker.h
    trackermanager.h
    kioannouncejob.h
)

install(FILES ${tracker_HDR} DESTINATION ${KDE_INSTALL_INCLUDEDIR}/libktorrent/tracker COMPONENT Devel)

if(HAVE_INSTALLED_LIBRARY_CHECK)
    ecm_installed_library_check_include_strings(KTorrent6 HEADERS ${tracker_HDR} PREFIX tracker)
endif()
