# Main executable
add_executable(cddsctl
    main.cpp
    commands/RecordCommand.cpp
    commands/ListCommand.cpp
    commands/InfoCommand.cpp
    commands/EchoCommand.cpp
)

target_include_directories(cddsctl PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(cddsctl PRIVATE
    cddsctl_recorder
    optionparser
    Threads::Threads
)

# Installation
install(TARGETS cddsctl
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
