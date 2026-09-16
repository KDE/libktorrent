if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

set(datachecker_HDR
    datacheckerthread.h
    datachecker.h
    singledatachecker.h
    multidatachecker.h
    datacheckerjob.h
)

install(FILES ${datachecker_HDR} DESTINATION ${KDE_INSTALL_INCLUDEDIR}/libktorrent/datachecker COMPONENT Devel)

if(HAVE_INSTALLED_LIBRARY_CHECK)
    ecm_installed_library_check_include_strings(KTorrent6 HEADERS ${datachecker_HDR} PREFIX datachecker)
endif()
