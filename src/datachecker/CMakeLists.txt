add_subdirectory(tests)

set(datachecker_HDR
    datacheckerthread.h
    datachecker.h
    singledatachecker.h
    multidatachecker.h
    datacheckerjob.h
)

install(FILES ${datachecker_HDR} DESTINATION ${INCLUDE_INSTALL_DIR}/libktorrent/datachecker COMPONENT Devel)
