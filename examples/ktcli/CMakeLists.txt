set(ktcli_SRC
    ktcli.cpp
    main.cpp
)

add_executable(ktcli ${ktcli_SRC})
target_link_libraries(ktcli ktorrent KF5::KIOCore KF5::Crash)
