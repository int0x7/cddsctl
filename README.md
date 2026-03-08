# cddsctl

[![License](https://img.shields.io/badge/License-Apache_2.0-blue.svg)](https://opensource.org/licenses/Apache-2.0) [![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://isocpp.org/std/the-standard) [![Platform](https://img.shields.io/badge/Platform-Linux-lightgrey.svg)](https://www.linux.org/) [![CycloneDDS](https://img.shields.io/badge/CycloneDDS-0.10.2-green.svg)](https://cyclonedds.io/) [![CI](https://github.com/int0x7/cddsctl/actions/workflows/ci.yml/badge.svg)](https://github.com/int0x7/cddsctl/actions) [![GitHub release](https://img.shields.io/github/v/release/int0x7/cddsctl)](https://github.com/int0x7/cddsctl/releases) [![GitHub stars](https://img.shields.io/github/stars/int0x7/cddsctl)](https://github.com/int0x7/cddsctl/stargazers)

**cddsctl** (Cyclone DDS Control) is a zero-config **DDS CLI tool** for topic discovery, real-time data inspection, and traffic recording — a `ros2 topic` / `ros2 bag` alternative that requires **no ROS dependency**. Built on **CycloneDDS** with **XTypes** introspection and **iceoryx** shared-memory support, it ships as a single statically-linked binary and writes recordings in the **MCAP** format.

[中文文档](README_zh.md)

```
$ cddsctl --help

    _____ _____  _____   _____  _____ _______ _
   / ____|  __ \|  __ \ / ____|/ ____|__   __| |
  | |    | |  | | |  | | (___ | |       | |  | |
  | |    | |  | | |  | |\___ \| |       | |  | |
  | |____| |__| | |__| |____) | |____   | |  | |____
   \_____|_____/|_____/|_____/ \_____|  |_|  |______|

  ╔══════════════════════════════════════════╗
  ║   Cyclone DDS Command Line Tool          ║
  ╚══════════════════════════════════════════╝

  Version: 1.0.0 (with SHM)

  Usage: cddsctl <command> [options]

  Commands:
    info      Show information about a DDS topic
    list      List available DDS topics
    echo      Print messages from a DDS topic
    record    Record DDS topics to MCAP file
    ps        Show DDS participants and applications

  Run 'cddsctl <command> --help' for more information.
```

Core Features:

- `list`: List discovered topics in DDS network
- `echo`: Print messages from a topic in real-time
- `record`: Record topics to **MCAP** files

---

## Features

- Native DDS (no ROS required)
- Built for CycloneDDS
- Unified CLI experience: `list / echo / record`
- Recording format: MCAP (easy playback, analysis, and visualization)
- Fully static linked binary (easy deployment)
- Ideal for debugging, integration testing, data collection, and issue reproduction

---

## Why cddsctl?

| | cddsctl | ros2 bag |
|---|---|---|
| ROS dependency | No | Yes |
| DDS implementation | CycloneDDS | Any (via ROS) |
| Single binary | Yes (static) | No (ROS workspace) |
| Auto type discovery | Yes (XTypes) | Via ROS type system |
| Output format | MCAP | db3 / MCAP |
| Shared memory | Yes (iceoryx) | Yes (iceoryx) |

---

## Use Cases

- **Robotics DDS debugging without ROS** — inspect and echo topics on any CycloneDDS network
- **Recording DDS traffic for offline analysis** — capture to MCAP, visualize in [Foxglove Studio](https://foxglove.dev)
- **Integration testing with DDS data capture** — record topic streams during CI or manual test runs
- **Headless / embedded deployment** — single static binary, no runtime dependencies

---

## Installation

### Download Release

Download pre-built binary from [Releases](https://github.com/int0x7/cddsctl/releases):

```bash
tar -xzf cddsctl-*-linux-x86_64.tar.gz
sudo mv cddsctl-*/bin/cddsctl /usr/local/bin/
cddsctl --help
```

The release binary is built with CycloneDDS 0.10.2 and iceoryx 2.0.5. Shared memory transport is used automatically when a compatible RouDi (iceoryx 2.0.5) daemon is running; otherwise it falls back to UDP network transport.

### Build from Source

#### Prerequisites

Install system dependencies:

```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install -y cmake g++ ninja-build git libacl1-dev

# RHEL/CentOS/Fedora
sudo yum install -y cmake gcc-c++ ninja-build git libacl-devel

# Arch Linux
sudo pacman -S cmake gcc ninja git acl
```

#### Build

```bash
git clone https://github.com/int0x7/cddsctl.git
cd cddsctl
./build.sh
```

The build script will automatically:
- Download and compile dependencies (yaml-cpp, iceoryx, CycloneDDS) as static libraries
- Build cddsctl with all dependencies statically linked

Build options:

```bash
./build.sh -t          # Build and run tests
./build.sh -c          # Clean build
./build.sh --clean-deps # Rebuild all dependencies
./build.sh -h          # Show help
```

After building, the binary will be available:

```
./build/cli/cddsctl
```

### Version Compatibility

cddsctl requires **CycloneDDS 0.10.1+** due to its dependency on the `dds_typeinfo_t` API for XTypes introspection.

| CycloneDDS Version | Support Status | Notes |
|-------------------|----------------|-------|
| 0.9.0 / 0.9.1 | ❌ Not supported | Missing `dds_typeinfo_t` API |
| 0.10.1 - 0.10.5 | ✅ Fully supported | Full API compatibility |
| 11.0.0 | ⚠️ Partial support | cddsctl builds; publisher builds may need iceoryx alignment |

---

## Quick Start

List DDS topics:

```bash
cddsctl list
```

Print topic data:

```bash
cddsctl echo /test/sensor
```

Record topic to MCAP:

```bash
cddsctl record /test/sensor -o log.mcap
```

Record multiple topics:

```bash
cddsctl record MotorState IMU CameraImage -o run.mcap
```

---

## Commands

### list

List discovered topics in DDS network.

```bash
cddsctl list
```

Example output:

```
MotorState
MotorCommand
IMU
CameraImage
```

---

### echo

Print messages from a topic in real-time. Output format is YAML (similar to `rostopic echo`).

```bash
cddsctl echo <topic> [options]
```

Options:

- `-n, --count=N`: Exit after printing N messages
- `-d, --domain=ID`: DDS domain ID (default: 0)
- `-t, --timeout=SEC`: Topic discovery timeout in seconds (default: 2.0)
- `--no-timestamp`: Don't show timestamps

Example:

```bash
cddsctl echo /test/sensor -n 5
```

Example output:

```yaml
---
[1772686402.862456386]
id: 21
timestamp: 2.102516088
values:
  - 0.861936
  - -0.507016
  - 0.21
name: sensor_1
```

---

### record

Record topics to **MCAP file**.

```bash
cddsctl record <topic...> -o <file.mcap>
```

Example:

```bash
cddsctl record MotorState -o motor.mcap
```

---

## Related Projects

- [CycloneDDS](https://github.com/eclipse-cyclonedds/cyclonedds) — the DDS implementation cddsctl is built on
- [iceoryx](https://github.com/eclipse-iceoryx/iceoryx) — zero-copy shared-memory transport
- [MCAP](https://mcap.dev) — open-source recording format used by cddsctl
- [Foxglove Studio](https://foxglove.dev) — visualize and replay MCAP files

---

## License

Apache License 2.0
