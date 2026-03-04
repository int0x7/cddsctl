#!/bin/bash
#
# Build iceoryx for cddsctl
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
THIRD_PARTY_DIR="$(dirname "${SCRIPT_DIR}")"

# Configuration
BUILD_DIR="${THIRD_PARTY_DIR}/build"
INSTALL_DIR="${THIRD_PARTY_DIR}/iceoryx"
VERSION="2.0.5"
TARBALL_URL="https://github.com/eclipse-iceoryx/iceoryx/archive/refs/tags/v${VERSION}.tar.gz"
JOBS=$(nproc)

echo "============================================================"
echo "Building iceoryx v${VERSION}"
echo "============================================================"
echo "BUILD_DIR:   ${BUILD_DIR}"
echo "INSTALL_DIR: ${INSTALL_DIR}"
echo ""

# Install dependencies
sudo apt-get update
sudo apt-get install -y cmake libacl1-dev libncurses5-dev build-essential wget

mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

# Download source
if [ -d "iceoryx-${VERSION}" ]; then
    echo "iceoryx-${VERSION} directory exists, skipping download"
else
    echo "Downloading iceoryx v${VERSION}..."
    wget -q --show-progress -O iceoryx-${VERSION}.tar.gz ${TARBALL_URL}
    tar -xzf iceoryx-${VERSION}.tar.gz
    rm iceoryx-${VERSION}.tar.gz
fi

cd "iceoryx-${VERSION}"

# Build
cmake -B build -S iceoryx_meta \
    -DCMAKE_INSTALL_PREFIX=${INSTALL_DIR} \
    -DBUILD_SHARED_LIBS=OFF \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON

cmake --build build -j${JOBS}
cmake --install build

echo ""
echo "============================================================"
echo "iceoryx build complete"
echo "============================================================"
echo "Install path: ${INSTALL_DIR}"
ls -la ${INSTALL_DIR}/lib/libiceoryx*.a 2>/dev/null || echo "Warning: static libraries not found"

# Clean up source directory
echo ""
echo "Cleaning up build directory..."
rm -rf "${BUILD_DIR}/iceoryx-${VERSION}"
echo "Deleted: ${BUILD_DIR}/iceoryx-${VERSION}"
