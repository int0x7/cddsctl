#!/bin/bash
#
# Build third-party dependencies as static libraries
#

set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
THIRD_PARTY="${PROJECT_ROOT}/3rd_party"
BUILD_DIR="${THIRD_PARTY}/build"
JOBS=$(nproc 2>/dev/null || echo 4)

# Detect architecture
ARCH=$(uname -m)
case "${ARCH}" in
    x86_64)
        ARCH_NAME="x86_64"
        ;;
    aarch64|arm64)
        ARCH_NAME="aarch64"
        ;;
    *)
        echo "Warning: Unsupported architecture '${ARCH}', defaulting to ${ARCH}"
        ARCH_NAME="${ARCH}"
        ;;
esac

# Cross-compilation support
CROSS_COMPILE=0
CROSS_ARCH=""

# Load versions from .versions file
VERSIONS_FILE="${PROJECT_ROOT}/.versions"
if [[ -f "${VERSIONS_FILE}" ]]; then
    source "${VERSIONS_FILE}"
else
    echo "Error: .versions file not found at ${VERSIONS_FILE}"
    exit 1
fi

# Install prefixes (architecture-specific)
ICEORYX_PREFIX="${THIRD_PARTY}/iceoryx-${ARCH_NAME}"
CYCLONEDDS_PREFIX="${THIRD_PARTY}/cyclonedds-${CYCLONEDDS_VERSION}-${ARCH_NAME}"
CYCLONEDDS_CXX_PREFIX="${THIRD_PARTY}/cyclonedds-cxx-${CYCLONEDDS_VERSION}-${ARCH_NAME}"
YAMLCPP_PREFIX="${THIRD_PARTY}/yaml-cpp-${ARCH_NAME}"

usage() {
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  -c, --clean     Clean build (remove build directory first)"
    echo "  -j N            Number of parallel jobs (default: $JOBS)"
    echo "  --arch ARCH     Target architecture for cross-compilation (x86_64, aarch64)"
    echo "  -h, --help      Show this help"
}

CLEAN=0

while [[ $# -gt 0 ]]; do
    case $1 in
        -c|--clean)
            CLEAN=1
            shift
            ;;
        -j)
            JOBS="$2"
            shift 2
            ;;
        --arch)
            CROSS_ARCH="$2"
            CROSS_COMPILE=1
            case "${CROSS_ARCH}" in
                x86_64|aarch64)
                    ARCH_NAME="${CROSS_ARCH}"
                    echo "==> Cross-compiling for ${ARCH_NAME}"
                    ;;
                *)
                    echo "Error: Unsupported architecture '${CROSS_ARCH}'"
                    echo "Supported architectures: x86_64, aarch64"
                    exit 1
                    ;;
            esac
            # Update prefixes with new architecture
            ICEORYX_PREFIX="${THIRD_PARTY}/iceoryx-${ARCH_NAME}"
            CYCLONEDDS_PREFIX="${THIRD_PARTY}/cyclonedds-${CYCLONEDDS_VERSION}-${ARCH_NAME}"
            CYCLONEDDS_CXX_PREFIX="${THIRD_PARTY}/cyclonedds-cxx-${CYCLONEDDS_VERSION}-${ARCH_NAME}"
            YAMLCPP_PREFIX="${THIRD_PARTY}/yaml-cpp-${ARCH_NAME}"
            shift 2
            ;;
        -h|--help)
            usage
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            usage
            exit 1
            ;;
    esac
done

if [[ $CLEAN -eq 1 ]]; then
    echo "==> Cleaning dependencies..."
    rm -rf "${BUILD_DIR}"
    rm -rf "${ICEORYX_PREFIX}"
    rm -rf "${CYCLONEDDS_PREFIX}"
    rm -rf "${CYCLONEDDS_CXX_PREFIX}"
    rm -rf "${YAMLCPP_PREFIX}"
fi

mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

# ============================================================================
# yaml-cpp
# ============================================================================
if [[ -f "${YAMLCPP_PREFIX}/lib/libyaml-cpp.a" ]]; then
    echo "==> yaml-cpp already built, skipping..."
else
    echo "==> Building yaml-cpp ${YAMLCPP_VERSION}..."

    if [[ ! -d "yaml-cpp" ]]; then
        git clone --depth 1 --branch ${YAMLCPP_VERSION} \
            https://github.com/jbeder/yaml-cpp.git
    fi

    cmake -S yaml-cpp -B yaml-cpp_build \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="${YAMLCPP_PREFIX}" \
        -DBUILD_SHARED_LIBS=OFF \
        -DYAML_CPP_BUILD_TESTS=OFF \
        -DYAML_CPP_BUILD_TOOLS=OFF \
        -DYAML_BUILD_SHARED_LIBS=OFF \
        -DCMAKE_POSITION_INDEPENDENT_CODE=ON

    cmake --build yaml-cpp_build -j${JOBS}
    cmake --install yaml-cpp_build

    echo "==> yaml-cpp installed to ${YAMLCPP_PREFIX}"
fi

# ============================================================================
# iceoryx
# ============================================================================
if [[ -f "${ICEORYX_PREFIX}/lib/libiceoryx_posh.a" ]]; then
    echo "==> iceoryx already built, skipping..."
else
    echo "==> Building iceoryx ${ICEORYX_VERSION}..."

    if [[ ! -d "iceoryx" ]]; then
        git clone --depth 1 --branch v${ICEORYX_VERSION} \
            https://github.com/eclipse-iceoryx/iceoryx.git
    fi

    cmake -S iceoryx/iceoryx_meta -B iceoryx_build \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="${ICEORYX_PREFIX}" \
        -DBUILD_SHARED_LIBS=OFF \
        -DROUDI_ENVIRONMENT=ON \
        -DEXAMPLES=OFF \
        -DBINDING_C=ON \
        -DINTROSPECTION=OFF \
        -DBUILD_TEST=OFF

    cmake --build iceoryx_build -j${JOBS}
    cmake --install iceoryx_build

    echo "==> iceoryx installed to ${ICEORYX_PREFIX}"
fi

# ============================================================================
# CycloneDDS
# ============================================================================
if [[ -f "${CYCLONEDDS_PREFIX}/lib/libddsc.a" ]]; then
    echo "==> CycloneDDS already built, skipping..."
else
    echo "==> Building CycloneDDS ${CYCLONEDDS_VERSION}..."

    if [[ ! -d "cyclonedds" ]]; then
        git clone --depth 1 --branch ${CYCLONEDDS_VERSION} \
            https://github.com/eclipse-cyclonedds/cyclonedds.git
    fi

    cmake -S cyclonedds -B cyclonedds_build \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="${CYCLONEDDS_PREFIX}" \
        -DCMAKE_PREFIX_PATH="${ICEORYX_PREFIX}" \
        -DBUILD_SHARED_LIBS=OFF \
        -DENABLE_SHM=ON \
        -DENABLE_SECURITY=OFF \
        -DENABLE_SSL=OFF \
        -DBUILD_IDLC=ON \
        -DBUILD_DDSPERF=OFF \
        -DBUILD_EXAMPLES=OFF

    cmake --build cyclonedds_build -j${JOBS}
    cmake --install cyclonedds_build

    echo "==> CycloneDDS installed to ${CYCLONEDDS_PREFIX}"
fi

# ============================================================================
# CycloneDDS-CXX
# ============================================================================
if [[ -f "${CYCLONEDDS_CXX_PREFIX}/lib/libddscxx.a" ]]; then
    echo "==> CycloneDDS-CXX already built, skipping..."
else
    echo "==> Building CycloneDDS-CXX ${CYCLONEDDS_VERSION}..."

    if [[ ! -d "cyclonedds-cxx" ]]; then
        git clone --depth 1 --branch ${CYCLONEDDS_VERSION} \
            https://github.com/eclipse-cyclonedds/cyclonedds-cxx.git
    fi

    cmake -S cyclonedds-cxx -B cyclonedds-cxx_build \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="${CYCLONEDDS_CXX_PREFIX}" \
        -DCMAKE_PREFIX_PATH="${ICEORYX_PREFIX};${CYCLONEDDS_PREFIX}" \
        -DBUILD_SHARED_LIBS=OFF \
        -DBUILD_EXAMPLES=OFF \
        -DBUILD_TESTING=OFF

    cmake --build cyclonedds-cxx_build -j${JOBS}
    cmake --install cyclonedds-cxx_build

    echo "==> CycloneDDS-CXX installed to ${CYCLONEDDS_CXX_PREFIX}"
fi

echo ""
echo "========================================="
echo "Dependencies built successfully!"
echo "========================================="
echo "Architecture:   ${ARCH_NAME}"
if [[ ${CROSS_COMPILE} -eq 1 ]]; then
    echo "Build type:     Cross-compiled"
fi
echo "yaml-cpp:       ${YAMLCPP_PREFIX}"
echo "iceoryx:        ${ICEORYX_PREFIX}"
echo "CycloneDDS:     ${CYCLONEDDS_PREFIX}"
echo "CycloneDDS-CXX: ${CYCLONEDDS_CXX_PREFIX}"
echo "========================================="
