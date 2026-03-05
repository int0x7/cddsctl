# cddsctl

[![License](https://img.shields.io/badge/License-Apache_2.0-blue.svg)](https://opensource.org/licenses/Apache-2.0) [![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://isocpp.org/std/the-standard) [![Platform](https://img.shields.io/badge/Platform-Linux-lightgrey.svg)](https://www.linux.org/) [![CycloneDDS](https://img.shields.io/badge/CycloneDDS-0.10.2-green.svg)](https://cyclonedds.io/) [![Build Status](https://github.com/int0x7/cddsctl/actions/workflows/build.yml/badge.svg)](https://github.com/int0x7/cddsctl/actions) [![GitHub release](https://img.shields.io/github/v/release/int0x7/cddsctl)](https://github.com/int0x7/cddsctl/releases) [![GitHub stars](https://img.shields.io/github/stars/int0x7/cddsctl)](https://github.com/int0x7/cddsctl/stargazers)

**cddsctl**（Cyclone DDS Control）是一个面向 **CycloneDDS** 的命令行工具，用于查看、打印和记录 DDS 数据，提供与 `ros2 topic` / `ros2 bag` 相近的使用体验，但 **不依赖 ROS**，专注于原生 DDS 数据空间。

[English](README.md)

核心功能：

- `list`：查看 DDS 网络中发现的 topic 列表
- `echo`：实时打印指定 topic 的消息内容
- `record`：将指定 topic 记录为 **MCAP** 文件

---

## 特性

- 原生 DDS（无需 ROS）
- 面向 CycloneDDS
- 统一 CLI 体验：`list / echo / record`
- 记录格式：MCAP（便于后续回放、分析、可视化）
- 适合调试、联调、数据采集与问题复现

---

## 安装

源码构建：

```bash
git clone https://github.com/<your-org>/cddsctl.git
cd cddsctl
mkdir -p build && cd build
cmake ..
cmake --build . -j
```

构建完成后将生成二进制：

```
./build/cli/cddsctl
```

---

## 快速开始

查看 DDS topic：

```bash
cddsctl list
```

打印 topic 数据：

```bash
cddsctl echo /test/sensor
```

记录 topic 为 MCAP：

```bash
cddsctl record /test/sensor -o log.mcap
```

记录多个 topic：

```bash
cddsctl record MotorState IMU CameraImage -o run.mcap
```

---

## 命令

### list

列出 DDS 网络中发现的 topic。

```bash
cddsctl list
```

示例输出：

```
MotorState
MotorCommand
IMU
CameraImage
```

---

### echo

实时打印指定 topic 的消息内容，输出格式为 YAML（类似 `rostopic echo`）。

```bash
cddsctl echo <topic> [options]
```

选项：

- `-n, --count=N`：打印 N 条消息后退出
- `-d, --domain=ID`：指定 DDS domain ID（默认：0）
- `-t, --timeout=SEC`：topic 发现超时时间（默认：2.0 秒）
- `--no-timestamp`：不显示时间戳

示例：

```bash
cddsctl echo /test/sensor -n 5
```

输出示例：

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

将指定 topic 记录为 **MCAP 文件**。

```bash
cddsctl record <topic...> -o <file.mcap>
```

示例：

```bash
cddsctl record MotorState -o motor.mcap
```

---

## License

Apache License 2.0
