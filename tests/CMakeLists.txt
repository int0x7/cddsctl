# Tests for cddsctl
cmake_minimum_required(VERSION 3.16)

# Catch2 header-only library
add_library(Catch2 INTERFACE)
target_include_directories(Catch2 INTERFACE ${CMAKE_SOURCE_DIR}/3rd_party/catch2)

# Test sources
set(TEST_SOURCES
    test_main.cpp
    test_guid.cpp
    test_topic_filter.cpp
    test_config.cpp
)

# Test executable
add_executable(cddsctl_tests ${TEST_SOURCES})

target_link_libraries(cddsctl_tests PRIVATE
    Catch2
    cddsctl_core
)

target_include_directories(cddsctl_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

# Enable testing
enable_testing()
add_test(NAME cddsctl_tests COMMAND cddsctl_tests)
