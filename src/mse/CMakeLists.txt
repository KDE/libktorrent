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

install(FILES ${mse_HDR} DESTINATION ${KDE_INSTALL_INCLUDEDIR_KF}/libktorrent/mse COMPONENT Devel)
