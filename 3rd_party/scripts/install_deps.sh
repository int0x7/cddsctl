#!/bin/bash
# Script to download and install third-party dependencies

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
DEPS_DIR="$(dirname "$SCRIPT_DIR")"

echo "=== Installing Third-party Dependencies ==="
echo ""

# MCAP
echo "[1/4] Installing MCAP library..."
cd "$DEPS_DIR/mcap"
if [ -d "temp" ]; then
    rm -rf temp
fi
git clone --depth 1 https://github.com/foxglove/mcap.git temp
rm -rf include
cp -r temp/cpp/mcap/include .
rm -rf temp
echo "MCAP installed successfully."
echo ""

# nlohmann-json
echo "[2/4] Installing nlohmann-json..."
cd "$DEPS_DIR/nlohmann-json"
mkdir -p include/nlohmann
curl -sL https://github.com/nlohmann/json/releases/download/v3.11.3/json.hpp -o include/nlohmann/json.hpp
echo "nlohmann-json installed successfully."
echo ""

# optionparser
echo "[3/4] Installing optionparser..."
cd "$DEPS_DIR/optionparser"
mkdir -p include
# Download from a reliable source
curl -sL "https://sourceforge.net/projects/optionparser/files/optionparser-1.7.tar.gz/download" -o op.tar.gz
tar -xzf op.tar.gz
cp optionparser-1.7/src/optionparser.h include/
rm -rf optionparser-1.7 op.tar.gz
echo "optionparser installed successfully."
echo ""

# spdlog
echo "[4/4] Installing spdlog..."
cd "$DEPS_DIR"
mkdir -p spdlog/include
curl -sL https://github.com/gabime/spdlog/archive/refs/tags/v1.12.0.tar.gz -o spdlog.tar.gz
tar -xzf spdlog.tar.gz
cp -r spdlog-1.12.0/include/spdlog spdlog/include/
rm -rf spdlog-1.12.0 spdlog.tar.gz
echo "spdlog installed successfully."
echo ""

echo "=== All dependencies installed successfully! ==="
echo ""
echo "You can now build the project:"
echo "  mkdir build && cd build"
echo "  cmake .."
echo "  make -j\$(nproc)"
