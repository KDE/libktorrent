#add_subdirectory(tests)

set (mse_HDR 
	./encryptedserverauthenticate.h
	./functions.h
	./rc4encryptor.h
	./encryptedauthenticate.h
	./bigint.h
	./encryptedpacketsocket.h
)

install(FILES ${mse_HDR} DESTINATION ${INCLUDE_INSTALL_DIR}/libktorrent/mse COMPONENT Devel)