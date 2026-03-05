# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

cddsctl (Cyclone DDS Control) is a command-line tool for CycloneDDS that provides `ros2 topic`/`ros2 bag`-like experience without ROS dependency. It operates directly on native DDS data space.

## Build Commands

```bash
# Full build
mkdir -p build && cd build && cmake .. && cmake --build . -j

# Rebuild after changes (from project root)
cmake --build build -j

# Run all tests
./build/tests/cddsctl_tests

# Run specific test by name pattern
./build/tests/cddsctl_tests "[config]"
./build/tests/cddsctl_tests "[guid]"
./build/tests/cddsctl_tests "[topic_filter]"

# Run CLI
./build/cli/cddsctl --help
./build/cli/cddsctl record --help
./build/cli/cddsctl echo --help

# Echo a topic (YAML output)
./build/cli/cddsctl echo /test/sensor -n 5
```

## Architecture

### Library Dependency Chain

```
cddsctl_core  →  cddsctl_dds  →  cddsctl_recorder  →  cddsctl (CLI)
    ↓                ↓                 ↓
 yaml-cpp      CycloneDDS         mcap, json
 spdlog        iceoryx
```

### Directory Layout

- `include/cddsctl/` - Public headers organized by module (core/, dds/, record/, cli/)
- `src/` - Implementation files matching header structure
- `cli/` - CLI entry point and subcommands
- `tests/` - Catch2 unit tests
- `3rd_party/` - Vendored dependencies

### Key Components

| Component | Location | Purpose |
|-----------|----------|---------|
| `Participant` | dds/ | Wraps CycloneDDS participant lifecycle |
| `TopicDiscovery` | dds/ | Monitors DCPSPublication/Subscription builtin topics |
| `RawDataReader` | dds/ | Extracts CDR data via `dds_takecdr()`, handles iceoryx SHM |
| `TypeSupport` | dds/ | XTypes introspection, generates IDL from TypeMapping |
| `YamlPrinter` | dds/ | Formats CDR data as YAML using XTypes TypeMapping |
| `Recorder` | record/ | Orchestrates topic discovery, readers, and MCAP output |
| `McapWriter` | record/ | Writes schemas, channels, and messages to MCAP |

### Namespaces

- `cddsctl::core` - Types, Config, Log
- `cddsctl::dds` - DDS abstraction
- `cddsctl::recorder` - Recording functionality
- `cddsctl::cli` - CLI commands

### Adding New Commands

1. Create `cli/commands/FooCommand.hpp` implementing `cddsctl::cli::Command`
2. Create `cli/commands/FooCommand.cpp` with `execute()` implementation
3. Register in `cli/main.cpp`

## Dependencies

All dependencies are vendored in `3rd_party/`:
- CycloneDDS (0.10.2) + iceoryx for SHM support
- MCAP for recording format
- nlohmann-json, spdlog, optionparser, yaml-cpp

## Language

C++17, CMake 3.16+
