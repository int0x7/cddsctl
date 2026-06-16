# Main executable
add_executable(cddsctl
    main.cpp
    commands/RecordCommand.cpp
    commands/ListCommand.cpp
    commands/InfoCommand.cpp
    commands/EchoCommand.cpp
    commands/PsCommand.cpp
    commands/HzCommand.cpp
)

target_include_directories(cddsctl PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Single source of truth for the CLI version: project(VERSION) in the top-level
# CMakeLists, injected so main.cpp doesn't carry its own copy.
target_compile_definitions(cddsctl PRIVATE CDDSCTL_VERSION="${PROJECT_VERSION}")

target_link_libraries(cddsctl PRIVATE
    cddsctl_recorder
    optionparser
    Threads::Threads
)

# Installation
install(TARGETS cddsctl
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
