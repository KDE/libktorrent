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

install(FILES ${datachecker_HDR} DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF}/libktorrent/datachecker COMPONENT Devel)
