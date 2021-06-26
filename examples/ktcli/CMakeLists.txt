add_executable(ktcli)

target_sources(ktcli PRIVATE
    ktcli.cpp
    main.cpp
)

target_link_libraries(ktcli KF5::Torrent)
