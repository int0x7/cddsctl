# Third-party Dependencies

This directory contains header-only third-party libraries used by the project.

## Required Libraries

### MCAP (mcap/)

MCAP is a modular container format for heterogeneous timestamped data.

**Version:** 1.3.0+
**Source:** https://github.com/foxglove/mcap
**License:** MIT

To install:
```bash
cd 3rd_party/mcap
git clone --depth 1 https://github.com/foxglove/mcap.git temp
cp -r temp/cpp/mcap/include .
rm -rf temp
```

### nlohmann-json (nlohmann-json/)

JSON for Modern C++ - a header-only JSON library.

**Version:** 3.11.0+
**Source:** https://github.com/nlohmann/json
**License:** MIT

To install:
```bash
cd 3rd_party/nlohmann-json
mkdir -p include/nlohmann
curl -L https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp -o include/nlohmann/json.hpp
```

### optionparser (optionparser/)

The Lean Mean C++ Option Parser - a header-only command line argument parser.

**Version:** 1.7
**Source:** https://optionparser.sourceforge.io/
**License:** MIT

To install:
```bash
cd 3rd_party/optionparser
mkdir -p include
curl -L https://raw.githubusercontent.com/afokin/optionparser/master/optionparser.h -o include/optionparser.h
```

### spdlog (spdlog/)

Fast C++ logging library - header-only.

**Version:** 1.12.0
**Source:** https://github.com/gabime/spdlog
**License:** MIT

To install:
```bash
cd 3rd_party
mkdir -p spdlog/include
curl -sL https://github.com/gabime/spdlog/archive/refs/tags/v1.12.0.tar.gz -o spdlog.tar.gz
tar -xzf spdlog.tar.gz
cp -r spdlog-1.12.0/include/spdlog spdlog/include/
rm -rf spdlog-1.12.0 spdlog.tar.gz
```

## Installation Script

Run the installation script to download all dependencies:

```bash
./3rd_party/scripts/install_deps.sh
```

## Alternative: System Package Installation

On some systems, these libraries may be available as packages:

### Ubuntu/Debian
```bash
# nlohmann-json
sudo apt install nlohmann-json3-dev

# For MCAP, use the source installation above
```

### Arch Linux
```bash
sudo pacman -S nlohmann-json
```

### macOS (Homebrew)
```bash
brew install nlohmann-json
```

When using system packages, modify the top-level CMakeLists.txt to use `find_package()` instead of the local 3rd_party directories.
