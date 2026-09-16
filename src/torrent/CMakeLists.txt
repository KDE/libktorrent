set(torrent_HDR
    torrent.h
    advancedchokealgorithm.h
    peersourcemanager.h
    choker.h
    server.h
    torrentcontrol.h
    uploader.h
    torrentcreator.h
    timeestimator.h
    torrentfile.h
    statsfile.h
    globals.h
    torrentstats.h
    job.h
    jobqueue.h
    torrentfilestream.h
)

install(FILES ${torrent_HDR} DESTINATION ${KDE_INSTALL_INCLUDEDIR}/libktorrent/torrent COMPONENT Devel)

if(HAVE_INSTALLED_LIBRARY_CHECK)
    ecm_installed_library_check_include_strings(KTorrent6 HEADERS ${torrent_HDR} PREFIX torrent)
endif()

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()
