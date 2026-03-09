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

  Version: 1.0.0 (with SHM)

  Usage: cddsctl <command> [options]

  Commands:
    hz        Display publishing frequency of a DDS topic
    info      Show information about a DDS topic
    list      List available DDS topics
    echo      Print messages from a DDS topic
    record    Record DDS topics to MCAP file
    ps        Show DDS participants and applications

  Run 'cddsctl <command> --help' for more information.
```

核心功能：

- `list`：查看 DDS 网络中发现的 topic 列表
- `echo`：实时打印指定 topic 的消息内容（支持 YAML/JSON 格式）
- `hz`：显示 topic 发布频率（类似 `rostopic hz`）
- `record`：将指定 topic 记录为 **MCAP** 文件
- **XTypes 类型自省**：无需 IDL 编译即可自动发现类型
- **共享内存**：通过 iceoryx 实现零拷贝（自动回退到 UDP）
- **复杂类型**：嵌套结构体、数组、序列、联合体、枚举

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
tar -xzf cddsctl-*-linux-x86_64.tar.gz
sudo mv cddsctl-*/bin/cddsctl /usr/local/bin/
cddsctl --help
```

发布版本使用 CycloneDDS 0.10.2 和 iceoryx 2.0.5 构建。当兼容的 RouDi (iceoryx 2.0.5) 守护进程运行时，会自动使用共享内存传输；否则会回退到 UDP 网络传输。

### 源码构建

#### 系统依赖

安装必要的系统依赖：

```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install -y cmake g++ ninja-build git libacl1-dev

# RHEL/CentOS/Fedora
sudo yum install -y cmake gcc-c++ ninja-build git libacl-devel

# Arch Linux
sudo pacman -S cmake gcc ninja git acl
```

#### 构建

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

### 版本兼容性

cddsctl 需要 **CycloneDDS 0.10.1+** 版本，因为它依赖 `dds_typeinfo_t` API 进行 XTypes 类型自省。

| CycloneDDS 版本 | 支持状态 | 说明 |
|----------------|---------|------|
| 0.9.0 / 0.9.1 | ❌ 不支持 | 缺少 `dds_typeinfo_t` API |
| 0.10.1 - 0.10.5 | ✅ 完全支持 | 完整 API 兼容性 |
| 11.0.0 | ⚠️ 部分支持 | cddsctl 可构建；publisher 构建可能需要 iceoryx 版本对齐 |

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

监控 topic 频率：

```bash
cddsctl hz /test/sensor
```

同时监控多个 topic 频率：

```bash
cddsctl hz /rt/imu_state /rt/joy
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
- `-F, --format=FMT`：输出格式：yaml、json（默认：yaml）
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

支持嵌套结构和联合体：

```yaml
---
[1772968360.232262]
timestamp_sec: 1772968360
base_pose:
  position:
    x: 0.198669
    y: 0.980066
    z: 0
  orientation:
    x: 0
    y: 0
    z: 0.099833
    w: 0.995004
result:
  _d: 0
  success: true
```

JSON 格式 (`-F json`)：

```json
{
  "base_pose": {
    "position": { "x": 0.198669, "y": 0.980066, "z": 0 },
    "orientation": { "x": 0, "y": 0, "z": 0.099833, "w": 0.995004 }
  },
  "overall_status": "STATUS_OK"
}
```

---

### hz

显示 DDS topic 的发布频率（类似 `rostopic hz`）。

```bash
cddsctl hz <topic...> [options]
```

选项：

- `-d, --domain=ID`：指定 DDS domain ID（默认：0）
- `-w, --window=N`：频率计算窗口大小（默认：100）
- `-t, --timeout=SEC`：topic 发现超时时间（默认：2.0 秒）

示例 - 单个 topic：

```bash
cddsctl hz /test/sensor
```

输出示例：

```
subscribed to [/test/sensor]
average rate: 59.987 Hz
    min: 59.823 Hz
    max: 60.156 Hz
    std dev: 0.092 Hz
    window: 100
```

示例 - 多个 topic：

```bash
cddsctl hz /rt/imu_state /rt/joy
```

输出示例：

```
subscribed to [/rt/imu_state]
subscribed to [/rt/joy]
[/rt/imu_state]
average rate: 498.738 Hz
    min: 411.489 Hz
    max: 580.234 Hz
    std dev: 25.153 Hz
    window: 100
[/rt/joy]
average rate: 60.001 Hz
    min: 59.768 Hz
    max: 60.192 Hz
    std dev: 0.085 Hz
    window: 100
---
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

## IDL 示例

本仓库包含全面的 IDL 示例用于测试：

| 示例 | 类型 | 说明 |
|---------|-------|-------------|
| `NestedStruct` | 结构体 | 层级结构（Vector3 → Pose → RobotState）|
| `ArraysAndSequences` | 数组、序列 | 固定数组、有界/无界序列 |
| `Enumeration` | 枚举 | 状态/模式的类型安全命名常量 |
| `UnionType` | 联合体 | 用于变体数据的判别式联合体 |
| `VariousTypes` | 基本类型 | 所有 DDS 基本类型 |
| `AdvancedFeatures` | 复杂类型 | 联合体和序列的深度嵌套 |

运行示例：

```bash
# 终端 1: 启动 publisher
./build/examples/nested_struct_publisher --topic /robot/state

# 终端 2: YAML 格式输出
./build/cli/cddsctl echo /robot/state -n 10

# 终端 2: JSON 格式输出
./build/cli/cddsctl echo /robot/state -n 10 --format json
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
