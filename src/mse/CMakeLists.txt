if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

set(mse_HDR
    encryptedserverauthenticate.h
    functions.h
    rc4encryptor.h
    encryptedauthenticate.h
    bigint.h
    encryptedpacketsocket.h
)

install(FILES ${mse_HDR} DESTINATION ${KDE_INSTALL_INCLUDEDIR}/libktorrent/mse COMPONENT Devel)

if(HAVE_INSTALLED_LIBRARY_CHECK)
    ecm_installed_library_check_include_strings(KTorrent6 HEADERS ${mse_HDR} PREFIX mse)
endif()
