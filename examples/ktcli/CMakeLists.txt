set(ktcli_SRC
    ktcli.cpp
    main.cpp
)

add_executable(ktcli ${ktcli_SRC})
target_link_libraries(ktcli KF5::Torrent)
