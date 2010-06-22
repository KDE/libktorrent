include_directories(${CMAKE_CURRENT_SOURCE_DIR}/../../src)

set(ktcli_SRC 
	ktcli.cpp
	main.cpp
)

kde4_add_executable(ktcli ${ktcli_SRC})
target_link_libraries(ktcli ktorrent ${KDE4_SOLID_LIBS} ${KDE4_KIO_LIBS})
