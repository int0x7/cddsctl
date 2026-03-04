#!/bin/bash
#
# Build CycloneDDS (with SHM enabled) for cddsctl
# Note: Run build_iceoryx.sh first
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
THIRD_PARTY_DIR="$(dirname "${SCRIPT_DIR}")"

# Configuration
BUILD_DIR="${THIRD_PARTY_DIR}/build"
ICEORYX_DIR="${THIRD_PARTY_DIR}/iceoryx"
VERSION="0.10.2"
CYCLONE_INSTALL_DIR="${THIRD_PARTY_DIR}/cyclonedds-${VERSION}"
CYCLONE_CXX_INSTALL_DIR="${THIRD_PARTY_DIR}/cyclonedds-cxx-${VERSION}"
JOBS=$(nproc)

# Download URLs
CYCLONE_URL="https://github.com/eclipse-cyclonedds/cyclonedds/archive/refs/tags/${VERSION}.tar.gz"
CYCLONE_CXX_URL="https://github.com/eclipse-cyclonedds/cyclonedds-cxx/archive/refs/tags/${VERSION}.tar.gz"

# Check iceoryx
if [ ! -d "${ICEORYX_DIR}/lib" ]; then
    echo "Error: iceoryx not installed, please run build_iceoryx.sh first"
    exit 1
fi

echo "============================================================"
echo "Building CycloneDDS ${VERSION} (SHM enabled)"
echo "============================================================"
echo "ICEORYX_DIR:            ${ICEORYX_DIR}"
echo "CYCLONE_INSTALL_DIR:    ${CYCLONE_INSTALL_DIR}"
echo "CYCLONE_CXX_INSTALL_DIR: ${CYCLONE_CXX_INSTALL_DIR}"
echo ""

mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

# === CycloneDDS C library ===
echo ">>> Building CycloneDDS C library..."

if [ -d "cyclonedds-${VERSION}" ]; then
    echo "cyclonedds-${VERSION} directory exists, skipping download"
else
    echo "Downloading cyclonedds ${VERSION}..."
    wget -q --show-progress -O cyclonedds-${VERSION}.tar.gz ${CYCLONE_URL}
    tar -xzf cyclonedds-${VERSION}.tar.gz
    rm cyclonedds-${VERSION}.tar.gz
fi

cd "cyclonedds-${VERSION}"

# Enable SHM only (consistent with existing configuration)
cmake -B build -S . \
    -DCMAKE_INSTALL_PREFIX=${CYCLONE_INSTALL_DIR} \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_SHARED_LIBS=ON \
    -DENABLE_TOPIC_DISCOVERY=ON \
    -DENABLE_TYPE_DISCOVERY=ON \
    -DENABLE_SECURITY=NO \
    -DENABLE_SSL=NO \
    -DENABLE_SHM=ON \
    -DBUILD_DOCS=OFF \
    -DBUILD_TESTING=OFF \
    -DBUILD_EXAMPLES=OFF \
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
    -DCMAKE_PREFIX_PATH=${ICEORYX_DIR}

cmake --build build -j${JOBS}
cmake --install build

cd "${BUILD_DIR}"

# === CycloneDDS C++ library ===
echo ""
echo ">>> Building CycloneDDS C++ library..."

if [ -d "cyclonedds-cxx-${VERSION}" ]; then
    echo "cyclonedds-cxx-${VERSION} directory exists, skipping download"
else
    echo "Downloading cyclonedds-cxx ${VERSION}..."
    wget -q --show-progress -O cyclonedds-cxx-${VERSION}.tar.gz ${CYCLONE_CXX_URL}
    tar -xzf cyclonedds-cxx-${VERSION}.tar.gz
    rm cyclonedds-cxx-${VERSION}.tar.gz
fi

cd "cyclonedds-cxx-${VERSION}"

cmake -B build -S . \
    -DCMAKE_INSTALL_PREFIX=${CYCLONE_CXX_INSTALL_DIR} \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_SHARED_LIBS=ON \
    -DBUILD_DOCS=OFF \
    -DBUILD_TESTING=OFF \
    -DBUILD_EXAMPLES=OFF \
    -DCMAKE_POSITION_INDEPENDENT_CODE=ON \
    -DCMAKE_PREFIX_PATH="${CYCLONE_INSTALL_DIR};${ICEORYX_DIR}"

cmake --build build -j${JOBS}
cmake --install build

echo ""
echo "============================================================"
echo "CycloneDDS (SHM) build complete"
echo "============================================================"
echo "CycloneDDS:     ${CYCLONE_INSTALL_DIR}"
echo "CycloneDDS-CXX: ${CYCLONE_CXX_INSTALL_DIR}"

# Clean up source directory
echo ""
echo "Cleaning up build directory..."
rm -rf "${BUILD_DIR}/cyclonedds-${VERSION}"
rm -rf "${BUILD_DIR}/cyclonedds-cxx-${VERSION}"
echo "Deleted build directories"

# Remove build directory if empty
rmdir "${BUILD_DIR}" 2>/dev/null && echo "Deleted: ${BUILD_DIR}" || true
