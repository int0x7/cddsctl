cmake_minimum_required(VERSION 3.16)
project(cddsctl VERSION 1.0.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Detect target architecture
if(CMAKE_SYSTEM_PROCESSOR)
    set(TARGET_ARCH "${CMAKE_SYSTEM_PROCESSOR}")
else()
    execute_process(COMMAND uname -m OUTPUT_VARIABLE TARGET_ARCH OUTPUT_STRIP_TRAILING_WHITESPACE)
endif()

# Normalize architecture names
if(TARGET_ARCH MATCHES "^(x86_64|amd64)$")
    set(TARGET_ARCH_NAME "x86_64")
elseif(TARGET_ARCH MATCHES "^(aarch64|arm64)$")
    set(TARGET_ARCH_NAME "aarch64")
else()
    set(TARGET_ARCH_NAME "${TARGET_ARCH}")
endif()

message(STATUS "Target architecture: ${TARGET_ARCH_NAME}")

# Debug symbols for all build types
add_compile_options(-g)

# Use GNUInstallDirs for standard install paths
include(GNUInstallDirs)

# Build options
option(BUILD_EXAMPLES "Build example programs" ON)

# Export compile commands for IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Third-party directory
set(CDDSCTL_3RD_PARTY ${CMAKE_CURRENT_SOURCE_DIR}/3rd_party)

# Load versions from .versions file
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/.versions" VERSION_LINES)
foreach(LINE ${VERSION_LINES})
  if(LINE MATCHES "^([A-Z_]+)=(.+)$")
    set(${CMAKE_MATCH_1} ${CMAKE_MATCH_2})
  endif()
endforeach()

# Third-party library paths (architecture-specific)
set(YAMLCPP_DIR ${CDDSCTL_3RD_PARTY}/yaml-cpp-${TARGET_ARCH_NAME})
set(ICEORYX_DIR ${CDDSCTL_3RD_PARTY}/iceoryx-${TARGET_ARCH_NAME})
set(CYCLONEDDS_DIR ${CDDSCTL_3RD_PARTY}/cyclonedds-${CYCLONEDDS_VERSION}-${TARGET_ARCH_NAME})
set(CYCLONEDDS_CXX_DIR ${CDDSCTL_3RD_PARTY}/cyclonedds-cxx-${CYCLONEDDS_VERSION}-${TARGET_ARCH_NAME})
list(APPEND CMAKE_PREFIX_PATH ${YAMLCPP_DIR} ${ICEORYX_DIR} ${CYCLONEDDS_DIR} ${CYCLONEDDS_CXX_DIR})

# CycloneDDS depends on iceoryx, need to find iceoryx targets first (static libs)
find_package(iceoryx_hoofs REQUIRED)
find_package(iceoryx_posh REQUIRED)
find_package(iceoryx_binding_c REQUIRED)
find_package(CycloneDDS REQUIRED)
find_package(Threads REQUIRED)
get_target_property(_ddsc_inc CycloneDDS::ddsc INTERFACE_INCLUDE_DIRECTORIES)
list(GET _ddsc_inc 0 _first_inc)
message(STATUS "Found CycloneDDS with headers at ${_first_inc}")

# Find yaml-cpp (prefer local static build)
set(yaml-cpp_DIR ${YAMLCPP_DIR}/lib/cmake/yaml-cpp)
find_package(yaml-cpp REQUIRED)

# MCAP library
add_library(mcap STATIC ${CDDSCTL_3RD_PARTY}/mcap/mcap_impl.cpp)
target_include_directories(mcap PUBLIC ${CDDSCTL_3RD_PARTY}/mcap/include)
target_compile_definitions(mcap PUBLIC MCAP_COMPRESSION_NO_LZ4 MCAP_COMPRESSION_NO_ZSTD)

# nlohmann-json (header-only)
add_library(nlohmann_json INTERFACE)
target_include_directories(nlohmann_json INTERFACE ${CDDSCTL_3RD_PARTY}/nlohmann-json/include)

# optionparser (header-only)
add_library(optionparser INTERFACE)
target_include_directories(optionparser INTERFACE ${CDDSCTL_3RD_PARTY}/optionparser/include)

# spdlog (header-only)
add_library(spdlog INTERFACE)
target_include_directories(spdlog INTERFACE ${CDDSCTL_3RD_PARTY}/spdlog/include)

# Add subdirectories
add_subdirectory(src)
add_subdirectory(cli)

if(BUILD_EXAMPLES)
    add_subdirectory(examples)
endif()

# Tests
option(BUILD_TESTING "Build tests" ON)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

# Summary
message(STATUS "")
message(STATUS "========================================")
message(STATUS "cddsctl Configuration")
message(STATUS "========================================")
message(STATUS "Version:        ${PROJECT_VERSION}")
message(STATUS "Architecture:   ${TARGET_ARCH_NAME}")
message(STATUS "Build Examples: ${BUILD_EXAMPLES}")
message(STATUS "========================================")
message(STATUS "")
