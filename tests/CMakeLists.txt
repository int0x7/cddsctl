# Tests for cddsctl
cmake_minimum_required(VERSION 3.16)

# Find CycloneDDS C++ bindings for IDL tests
find_package(CycloneDDS-CXX QUIET)

# Catch2 header-only library
add_library(Catch2 INTERFACE)
target_include_directories(Catch2 INTERFACE ${CMAKE_SOURCE_DIR}/3rd_party/catch2)
target_compile_features(Catch2 INTERFACE cxx_std_17)

# ============================================================================
# Core Library Tests
# ============================================================================

set(TEST_SOURCES
    test_main.cpp
    test_guid.cpp
    test_topic_filter.cpp
    test_config.cpp
)

add_executable(cddsctl_tests ${TEST_SOURCES})

target_link_libraries(cddsctl_tests PRIVATE
    Catch2
    cddsctl_core
)

target_include_directories(cddsctl_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

add_test(NAME cddsctl_tests COMMAND cddsctl_tests)

# ============================================================================
# IDL Type Tests (require CycloneDDS-CXX)
# ============================================================================

if(CycloneDDS-CXX_FOUND)
    # Generate IDL types for tests (reuse examples IDL)
    idlcxx_generate(
        TARGET TestIdlNestedStruct
        FILES ${CMAKE_SOURCE_DIR}/examples/idl/NestedStruct.idl
    )

    idlcxx_generate(
        TARGET TestIdlArrays
        FILES ${CMAKE_SOURCE_DIR}/examples/idl/ArraysAndSequences.idl
    )

    idlcxx_generate(
        TARGET TestIdlVariousTypes
        FILES ${CMAKE_SOURCE_DIR}/examples/idl/VariousTypes.idl
    )

    idlcxx_generate(
        TARGET TestIdlEnumeration
        FILES ${CMAKE_SOURCE_DIR}/examples/idl/Enumeration.idl
    )

    idlcxx_generate(
        TARGET TestIdlUnionType
        FILES ${CMAKE_SOURCE_DIR}/examples/idl/UnionType.idl
    )

    idlcxx_generate(
        TARGET TestIdlAdvancedFeatures
        FILES ${CMAKE_SOURCE_DIR}/examples/idl/AdvancedFeatures.idl
    )

    # IDL basic type tests
    add_executable(idl_basic_tests
        idl/test_idl_basic.cpp
    )

    target_link_libraries(idl_basic_tests PRIVATE
        Catch2
        TestIdlNestedStruct
        TestIdlArrays
        TestIdlVariousTypes
        TestIdlEnumeration
        TestIdlUnionType
        TestIdlAdvancedFeatures
        CycloneDDS-CXX::ddscxx
    )

    target_include_directories(idl_basic_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_CURRENT_BINARY_DIR}  # For generated IDL headers
    )

    add_test(NAME idl_basic_tests COMMAND idl_basic_tests)

    # IDL YAML formatting tests
    add_executable(idl_yaml_tests
        idl/test_idl_yaml.cpp
    )

    target_link_libraries(idl_yaml_tests PRIVATE
        Catch2
        TestIdlNestedStruct
        TestIdlArrays
        TestIdlEnumeration
        CycloneDDS-CXX::ddscxx
    )

    target_include_directories(idl_yaml_tests PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_CURRENT_BINARY_DIR}
    )

    add_test(NAME idl_yaml_tests COMMAND idl_yaml_tests)

else()
    message(WARNING "CycloneDDS-CXX not found, IDL tests will not be built")
endif()

# ============================================================================
# Integration Tests
# ============================================================================

# Copy integration test script to build directory
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/integration/test_idl_integration.sh
    ${CMAKE_CURRENT_BINARY_DIR}/test_idl_integration.sh
    COPYONLY
)

# Add integration test (requires built examples)
add_test(
    NAME idl_integration_test
    COMMAND ${CMAKE_CURRENT_BINARY_DIR}/test_idl_integration.sh
)
# Integration tests depend on examples being built
set_tests_properties(idl_integration_test PROPERTIES
    DEPENDS "nested_struct_publisher;array_publisher;various_types_publisher;enum_publisher;union_publisher;advanced_publisher"
    DISABLED $<NOT:$<TARGET_EXISTS:nested_struct_publisher>>
)
