# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

cddsctl (Cyclone DDS Control) is a command-line tool for CycloneDDS that provides `ros2 topic`/`ros2 bag`-like experience without ROS dependency. It operates directly on native DDS data space.

## Build Commands

```bash
# One-command build (auto-builds dependencies if missing)
./build.sh

# Build with tests
./build.sh -t

# Clean build
./build.sh -c

# Rebuild dependencies from source
./build.sh --clean-deps

# Manual build (after dependencies are ready)
cmake --build build -j

# Run all tests
./build/tests/cddsctl_tests

# Run specific test by name pattern (Catch2 tag syntax)
./build/tests/cddsctl_tests "[config]"
./build/tests/cddsctl_tests "[guid]"
./build/tests/cddsctl_tests "[topic_filter]"

# Run CLI
./build/cli/cddsctl --help
./build/cli/cddsctl list
./build/cli/cddsctl info /test/sensor
./build/cli/cddsctl echo /test/sensor -n 5
./build/cli/cddsctl record /test/sensor -o log.mcap

# Run test publishers (for testing echo/record)
./build/examples/test_publisher --topic /test/sensor --rate 10
./build/examples/nested_struct_publisher --topic /robot/state --rate 1
./build/examples/union_publisher --topic /command/response --rate 1

# Run IDL tests
./build/tests/idl_basic_tests
./build/tests/idl_yaml_tests

# Run integration tests
./tests/integration/test_idl_integration.sh

# Run version compatibility tests
python3 tests/test_version_compat.py 0.10.2 0.10.5  # Test cddsctl 0.10.2 with publisher 0.10.5
python3 tests/test_version_compat.py --test-all      # Test all version combinations
```

## Architecture

### Source Layout

- `include/cddsctl/` - Public headers (mirrors namespace structure: `core/`, `dds/`, `cli/`, `record/`)
- `src/` - Library implementations (three static libs: `cddsctl_core`, `cddsctl_dds`, `cddsctl_recorder`)
- `cli/` - CLI binary and subcommand implementations (`cli/commands/`)
- `tests/` - Catch2 unit tests (`tests/idl/` for IDL tests)
- `examples/` - Test publishers (`examples/idl/` for IDL definitions)
- `3rd_party/` - Header-only libs (mcap, nlohmann-json, spdlog, optionparser, catch2) and built deps

### Library Dependency Chain

```
cddsctl_core  ->  cddsctl_dds  ->  cddsctl_recorder  ->  cddsctl (CLI)
    |                |                 |
 yaml-cpp      CycloneDDS         mcap, json
 spdlog        iceoryx
```

### Key Components

| Component | Location | Purpose |
|-----------|----------|---------|
| `Participant` | dds/ | Wraps CycloneDDS participant lifecycle |
| `TopicDiscovery` | dds/ | Monitors DCPSPublication/Subscription builtin topics |
| `RawDataReader` | dds/ | Extracts CDR data via `dds_takecdr()`, handles iceoryx SHM |
| `TypeSupport` | dds/ | XTypes introspection, generates IDL from TypeMapping |
| `YamlPrinter` | dds/ | Formats CDR data as YAML (supports nested structs, unions, enums) |
| `JsonPrinter` | dds/ | Formats CDR data as JSON (supports nested structs, unions, enums) |
| `CdrReader` | dds/ | Helper functions for CDR deserialization and type lookup |
| `Recorder` | record/ | Orchestrates topic discovery, readers, and MCAP output |
| `McapWriter` | record/ | Writes schemas, channels, and messages to MCAP |

### Namespaces

- `cddsctl::core` - Types, Config, Log
- `cddsctl::dds` - DDS abstraction (XTypes introspection, CDR parsing)
- `cddsctl::recorder` - Recording functionality
- `cddsctl::cli` - CLI commands

### IDL Type Support

cddsctl supports all OMG IDL 4.2 types via XTypes introspection:

| Category | Supported Types |
|----------|-----------------|
| Primitives | boolean, byte, char, short, long, long long, float, double |
| Strings | string (unbounded/bounded) |
| Structs | Nested structs with arbitrary depth |
| Arrays | Fixed-size arrays (e.g., `double[3][3]`) |
| Sequences | Unbounded/bounded sequences |
| Unions | Discriminated unions with enum/integer discriminator |
| Enums | Enumeration types with named values |

Type discovery is automatic via DDS XTypes - no IDL compilation needed for cddsctl.

### Adding New Commands

1. Create `cli/commands/FooCommand.hpp` implementing `cddsctl::cli::Command` (virtual interface in `include/cddsctl/cli/Command.hpp`)
2. Create `cli/commands/FooCommand.cpp` with `execute()` implementation
3. Register in `cli/main.cpp` via the `get_commands()` registry map

## Dependencies

Built as static libraries via `scripts/build_deps.sh`:
- yaml-cpp (0.8.0)
- iceoryx (2.0.3) for SHM support
- CycloneDDS (0.10.2)
- CycloneDDS-CXX (0.10.2)

Header-only libraries in `3rd_party/`:
- MCAP, nlohmann-json, spdlog, optionparser, Catch2

## Language

C++17, CMake 3.16+
