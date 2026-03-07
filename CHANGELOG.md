# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [0.0.1] - 2025-03-05

### Added

- **CLI Commands**
  - `list` - List all discovered DDS topics
  - `echo` - Subscribe and print topic messages (YAML/JSON format)
  - `info` - Show topic type information
  - `record` - Record topics to MCAP file format
  - `ps` - Show DDS participants with SHM status

- **DDS Features**
  - Native CycloneDDS support without ROS dependency
  - XTypes introspection for automatic type discovery
  - Iceoryx shared memory transport support
  - CDR serialization/deserialization

- **Build System**
  - One-command build script (`build.sh`)
  - Static linking for portable binaries
  - GitHub Actions CI/CD pipeline

- **Examples**
  - Test publisher for end-to-end testing

### Dependencies

- CycloneDDS 0.10.2
- CycloneDDS-CXX 0.10.2
- Iceoryx 2.0.3
- yaml-cpp 0.8.0
