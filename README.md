# cddsctl

[![License](https://img.shields.io/badge/License-Apache_2.0-blue.svg)](https://opensource.org/licenses/Apache-2.0) [![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://isocpp.org/std/the-standard) [![Platform](https://img.shields.io/badge/Platform-Linux-lightgrey.svg)](https://www.linux.org/) [![CycloneDDS](https://img.shields.io/badge/CycloneDDS-0.10.2-green.svg)](https://cyclonedds.io/) [![CI](https://github.com/int0x7/cddsctl/actions/workflows/ci.yml/badge.svg)](https://github.com/int0x7/cddsctl/actions) [![GitHub release](https://img.shields.io/github/v/release/int0x7/cddsctl)](https://github.com/int0x7/cddsctl/releases) [![GitHub stars](https://img.shields.io/github/stars/int0x7/cddsctl)](https://github.com/int0x7/cddsctl/stargazers)

**cddsctl** (Cyclone DDS Control) is a command-line tool for **CycloneDDS** that provides functionality similar to `ros2 topic` / `ros2 bag`, but **without ROS dependency**. It operates directly on native DDS data space.

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

  Version: 1.0.0

  Usage: cddsctl <command> [options]

  Commands:
    info      Show information about a DDS topic
    list      List available DDS topics
    echo      Print messages from a DDS topic
    record    Record DDS topics to MCAP file

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

## Installation

### Download Release

Download pre-built binary from [Releases](https://github.com/int0x7/cddsctl/releases):

```bash
tar -xzf cddsctl-*.tar.gz
./cddsctl-*/bin/cddsctl --help
```

### Build from Source

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

## License

Apache License 2.0
