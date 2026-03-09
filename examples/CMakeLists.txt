# Examples CMakeLists.txt
# Test publishers for cddsctl end-to-end testing

# Find CycloneDDS C++ bindings
find_package(CycloneDDS-CXX REQUIRED)

# ============================================================================
# IDL Files
# ============================================================================

# Original simple test types
idlcxx_generate(
    TARGET TestTypes
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/idl/TestTypes.idl
)

# Nested structures (demonstrates hierarchy)
idlcxx_generate(
    TARGET NestedStructTypes
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/idl/NestedStruct.idl
)

# Arrays and sequences
idlcxx_generate(
    TARGET ArrayTypes
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/idl/ArraysAndSequences.idl
)

# Various primitive types
idlcxx_generate(
    TARGET VariousTypes
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/idl/VariousTypes.idl
)

# Enumerations
idlcxx_generate(
    TARGET EnumTypes
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/idl/Enumeration.idl
)

# Union types
idlcxx_generate(
    TARGET UnionTypes
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/idl/UnionType.idl
)

# Advanced features
idlcxx_generate(
    TARGET AdvancedTypes
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/idl/AdvancedFeatures.idl
)

# ============================================================================
# Publisher Executables
# ============================================================================

# Original simple test publisher
add_executable(test_publisher
    test_publisher.cpp
)
target_link_libraries(test_publisher PRIVATE
    TestTypes
    CycloneDDS-CXX::ddscxx
    Threads::Threads
)

# Publisher for nested structures
add_executable(nested_struct_publisher
    nested_struct_publisher.cpp
)
target_link_libraries(nested_struct_publisher PRIVATE
    NestedStructTypes
    CycloneDDS-CXX::ddscxx
    Threads::Threads
)

# Publisher for arrays
add_executable(array_publisher
    array_publisher.cpp
)
target_link_libraries(array_publisher PRIVATE
    ArrayTypes
    CycloneDDS-CXX::ddscxx
    Threads::Threads
)

# Publisher for various types
add_executable(various_types_publisher
    various_types_publisher.cpp
)
target_link_libraries(various_types_publisher PRIVATE
    VariousTypes
    CycloneDDS-CXX::ddscxx
    Threads::Threads
)

# Publisher for enums
add_executable(enum_publisher
    enum_publisher.cpp
)
target_link_libraries(enum_publisher PRIVATE
    EnumTypes
    CycloneDDS-CXX::ddscxx
    Threads::Threads
)

# Publisher for unions
add_executable(union_publisher
    union_publisher.cpp
)
target_link_libraries(union_publisher PRIVATE
    UnionTypes
    CycloneDDS-CXX::ddscxx
    Threads::Threads
)

# Publisher for advanced features
add_executable(advanced_publisher
    advanced_publisher.cpp
)
target_link_libraries(advanced_publisher PRIVATE
    AdvancedTypes
    CycloneDDS-CXX::ddscxx
    Threads::Threads
)

# ============================================================================
# Installation
# ============================================================================

install(TARGETS
    test_publisher
    nested_struct_publisher
    array_publisher
    various_types_publisher
    enum_publisher
    union_publisher
    advanced_publisher
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
