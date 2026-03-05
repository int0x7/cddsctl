# Core library
add_library(cddsctl_core STATIC
    core/Types.cpp
    core/Config.cpp
)
target_include_directories(cddsctl_core PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(cddsctl_core PUBLIC yaml-cpp::yaml-cpp spdlog)

# DDS library
add_library(cddsctl_dds STATIC
    dds/Participant.cpp
    dds/TopicDiscovery.cpp
    dds/RawDataReader.cpp
    dds/TypeSupport.cpp
    dds/YamlPrinter.cpp
)
target_include_directories(cddsctl_dds PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(cddsctl_dds PUBLIC
    cddsctl_core
    CycloneDDS::ddsc
    iceoryx_binding_c::iceoryx_binding_c
    iceoryx_posh::iceoryx_posh
    iceoryx_hoofs::iceoryx_hoofs
    Threads::Threads
)

# Recorder library
add_library(cddsctl_recorder STATIC
    record/Recorder.cpp
    record/McapWriter.cpp
    record/TypesCollection.cpp
    record/Configuration.cpp
)
target_include_directories(cddsctl_recorder PUBLIC
    $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
target_link_libraries(cddsctl_recorder PUBLIC
    cddsctl_dds
    mcap
    nlohmann_json
)
