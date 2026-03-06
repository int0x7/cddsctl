# cddsctl

[![License](https://img.shields.io/badge/License-Apache_2.0-blue.svg)](https://opensource.org/licenses/Apache-2.0) [![C++17](https://img.shields.io/badge/C%2B%2B-17-blue.svg)](https://isocpp.org/std/the-standard) [![Platform](https://img.shields.io/badge/Platform-Linux-lightgrey.svg)](https://www.linux.org/) [![CycloneDDS](https://img.shields.io/badge/CycloneDDS-0.10.2-green.svg)](https://cyclonedds.io/) [![CI](https://github.com/int0x7/cddsctl/actions/workflows/ci.yml/badge.svg)](https://github.com/int0x7/cddsctl/actions) [![GitHub release](https://img.shields.io/github/v/release/int0x7/cddsctl)](https://github.com/int0x7/cddsctl/releases) [![GitHub stars](https://img.shields.io/github/stars/int0x7/cddsctl)](https://github.com/int0x7/cddsctl/stargazers)

**cddsctl**（Cyclone DDS Control）是一个零配置的 **DDS 命令行工具**，支持 topic 自动发现、实时数据查看和流量录制，提供与 `ros2 topic` / `ros2 bag` 相近的使用体验，但 **不依赖 ROS**。基于 **CycloneDDS** 构建，支持 **XTypes** 类型自省和 **iceoryx** 共享内存传输，以单个静态链接二进制文件交付，录制格式为 **MCAP**。

[English](README.md)

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
- 完全静态链接（便于部署）
- 适合调试、联调、数据采集与问题复现

---

## 为什么选择 cddsctl？

| | cddsctl | ros2 bag |
|---|---|---|
| ROS 依赖 | 无 | 是 |
| DDS 实现 | CycloneDDS | 任意（通过 ROS） |
| 单一二进制 | 是（静态链接） | 否（需要 ROS 工作空间） |
| 自动类型发现 | 是（XTypes） | 通过 ROS 类型系统 |
| 输出格式 | MCAP | db3 / MCAP |
| 共享内存 | 是（iceoryx） | 是（iceoryx） |

---

## 使用场景

- **无 ROS 环境下的 DDS 调试** — 在任意 CycloneDDS 网络上查看和打印 topic
- **录制 DDS 流量用于离线分析** — 录制为 MCAP，使用 [Foxglove Studio](https://foxglove.dev) 可视化
- **集成测试中的 DDS 数据采集** — 在 CI 或手动测试中录制 topic 数据流
- **无头/嵌入式部署** — 单个静态二进制，无运行时依赖

---

## 安装

### 下载发布版本

从 [Releases](https://github.com/int0x7/cddsctl/releases) 下载预编译二进制：

```bash
tar -xzf cddsctl-*.tar.gz
./cddsctl-*/bin/cddsctl --help
```

### 源码构建

```bash
git clone https://github.com/int0x7/cddsctl.git
cd cddsctl
./build.sh
```

构建脚本会自动：
- 下载并编译依赖（yaml-cpp, iceoryx, CycloneDDS）为静态库
- 静态链接所有依赖构建 cddsctl

构建选项：

```bash
./build.sh -t          # 构建并运行测试
./build.sh -c          # 清理后重新构建
./build.sh --clean-deps # 重新编译所有依赖
./build.sh -h          # 显示帮助
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

## 相关项目

- [CycloneDDS](https://github.com/eclipse-cyclonedds/cyclonedds) — cddsctl 所基于的 DDS 实现
- [iceoryx](https://github.com/eclipse-iceoryx/iceoryx) — 零拷贝共享内存传输
- [MCAP](https://mcap.dev) — cddsctl 使用的开源录制格式
- [Foxglove Studio](https://foxglove.dev) — 可视化和回放 MCAP 文件

---

## License

Apache License 2.0
