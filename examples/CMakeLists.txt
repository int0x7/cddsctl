# Examples CMakeLists.txt
# Test publisher for cddsctl record end-to-end testing

# Find CycloneDDS C++ bindings
find_package(CycloneDDS-CXX REQUIRED)

# Generate C++ types from IDL
idlcxx_generate(
    TARGET TestTypes
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/TestTypes.idl
)

# Test publisher executable
add_executable(test_publisher
    test_publisher.cpp
)

target_link_libraries(test_publisher PRIVATE
    TestTypes
    CycloneDDS-CXX::ddscxx
)

# Install
install(TARGETS test_publisher
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
