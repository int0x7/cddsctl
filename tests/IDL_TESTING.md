# IDL Testing Guide

This document describes the test suite for the IDL example types in cddsctl.

## Overview

The IDL test suite consists of three categories:

1. **Unit Tests** - Test IDL type construction, field access, and operations
2. **YAML Output Tests** - Verify YAML formatting for various IDL constructs
3. **Integration Tests** - End-to-end testing with cddsctl echo/record

## IDL Types Tested

| IDL File | Key Features | Test File |
|----------|-------------|-----------|
| `NestedStruct.idl` | Hierarchical nested structures (Vector3 → Pose → RobotState) | `test_idl_basic.cpp` |
| `ArraysAndSequences.idl` | Fixed arrays, unbounded sequences, bounded sequences | `test_idl_basic.cpp` |
| `VariousTypes.idl` | All primitive types (short, long, long long, float, double, char, wchar, etc.) | `test_idl_basic.cpp` |
| `Enumeration.idl` | Enum types with and without explicit values | `test_idl_basic.cpp` |
| `UnionType.idl` | Discriminated unions (variant types) | `test_idl_basic.cpp` |
| `AdvancedFeatures.idl` | Complex nested structures, unions in sequences, metadata | `test_idl_basic.cpp` |

## Running Tests

### Build Tests

```bash
# Build all tests
cmake --build build --target idl_basic_tests idl_yaml_tests

# Build integration test dependencies
cmake --build build --target nested_struct_publisher array_publisher \
    various_types_publisher enum_publisher union_publisher advanced_publisher
```

### Run Unit Tests

```bash
# Run core tests
./build/tests/cddsctl_tests

# Run IDL basic type tests
./build/tests/idl_basic_tests

# Run IDL YAML formatting tests
./build/tests/idl_yaml_tests

# Run specific test category
./build/tests/idl_basic_tests "[idl][nested]"
./build/tests/idl_basic_tests "[idl][arrays]"
./build/tests/idl_basic_tests "[idl][unions]"
```

### Run Integration Tests

```bash
# Run all integration tests
./build/tests/test_idl_integration.sh

# Or using ctest
ctest -R idl_integration_test -V
```

## Test Structure

### Unit Tests (`test_idl_basic.cpp`)

Tests basic operations on IDL-generated C++ types:

```cpp
TEST_CASE("NestedStruct basic operations", "[idl][nested]") {
    SECTION("Vector3 creation and access") {
        Vector3 vec;
        vec.x(1.0);
        vec.y(2.0);
        vec.z(3.0);

        REQUIRE(vec.x() == Approx(1.0));
        // ...
    }
}
```

### YAML Tests (`test_idl_yaml.cpp`)

Tests YAML formatting for IDL types:

```cpp
TEST_CASE("YAML formatting for nested structures", "[idl][yaml][nested]") {
    SECTION("Pose YAML output with nesting") {
        Pose pose;
        // ... set values
        std::string yaml = to_yaml(pose);
        REQUIRE(yaml.find("position:") != std::string::npos);
        REQUIRE(yaml.find("  x: 1") != std::string::npos);
    }
}
```

### Integration Tests (`test_idl_integration.sh`)

End-to-end tests that:
1. Start an IDL publisher
2. Run `cddsctl echo` to verify display
3. Run `cddsctl record` to verify MCAP output

## Adding New IDL Tests

### 1. Add IDL File

Create `examples/idl/YourType.idl`:

```idl
module demo {
    module test {
        @final
        struct YourType {
            long id;
            string name;
        };
    };
};
```

### 2. Add Publisher (Optional)

Create `examples/your_type_publisher.cpp`:

```cpp
#include "YourType.hpp"
// ... publisher implementation
```

### 3. Add Unit Tests

Add to `tests/idl/test_idl_basic.cpp`:

```cpp
TEST_CASE("YourType operations", "[idl][yourtype]") {
    SECTION("Creation and access") {
        demo::test::YourType obj;
        obj.id(42);
        obj.name("test");
        REQUIRE(obj.id() == 42);
    }
}
```

### 4. Update CMakeLists.txt

Add IDL generation in `tests/CMakeLists.txt`:

```cmake
idlcxx_generate(
    TARGET TestIdlYourType
    FILES ${CMAKE_SOURCE_DIR}/examples/idl/YourType.idl
)

target_link_libraries(idl_basic_tests PRIVATE
    # ... existing targets ...
    TestIdlYourType
)
```

## Test Coverage

### Nested Structures

- ✅ Multi-level nesting (Vector3 → Pose → RobotState)
- ✅ Sequence of nested structs (joints in RobotState)
- ✅ Deep nesting (3+ levels)

### Arrays and Sequences

- ✅ Fixed-size 1D arrays (e.g., `double[3]`)
- ✅ Fixed-size 2D arrays (e.g., `double[3][3]`)
- ✅ Unbounded sequences (`sequence<double>`)
- ✅ Bounded sequences (`sequence<double, 10>`)
- ✅ Sequence of fixed arrays
- ✅ 2D sequences (sequence of sequences)

### Primitive Types

- ✅ Signed integers: short, long, long long
- ✅ Unsigned integers: unsigned short, unsigned long, unsigned long long
- ✅ Floating point: float, double, long double
- ✅ Characters: char, wchar
- ✅ Other: boolean, octet
- ✅ Strings: unbounded, bounded
- ✅ Wide strings: unbounded, bounded

### Enums

- ✅ Basic enums
- ✅ Enums with explicit values
- ✅ Enums in struct fields

### Unions

- ✅ Discriminated unions
- ✅ Union with different types (int, float, string, binary)
- ✅ Union in struct fields
- ✅ Sequence of unions

### Advanced Features

- ✅ Nested struct sequences
- ✅ Union sequences
- ✅ Metadata with key-value pairs
- ✅ Complex camera calibration structures
- ✅ Device hierarchy with sub-devices

## Troubleshooting

### IDL Generation Fails

Ensure CycloneDDS-CXX is installed and found:
```bash
cmake -B build -DCycloneDDS-CXX_DIR=/path/to/cyclonedds-cxx/lib/cmake/CycloneDDS-CXX
```

### Integration Test Times Out

Check that:
1. All publishers are built: `cmake --build build --target all`
2. RouDi is not running (for SHM tests): `pkill iox-roudi`
3. No conflicting DDS applications on test domains

### Test Discovery

If tests aren't discovered:
```bash
ctest --output-on-failure -R idl
```
