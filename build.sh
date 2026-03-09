#!/bin/bash

set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"
THIRD_PARTY="${PROJECT_ROOT}/3rd_party"

# Load versions from .versions file
VERSIONS_FILE="${PROJECT_ROOT}/.versions"
if [[ -f "${VERSIONS_FILE}" ]]; then
    source "${VERSIONS_FILE}"
else
    echo "Error: .versions file not found at ${VERSIONS_FILE}"
    exit 1
fi

# 默认并行数
JOBS=$(nproc 2>/dev/null || echo 4)

usage() {
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  -c, --clean       Clean build (remove build directory first)"
    echo "  -d, --deps        Build dependencies (iceoryx + CycloneDDS)"
    echo "  --clean-deps      Clean and rebuild dependencies"
    echo "  -j N              Number of parallel jobs (default: $JOBS)"
    echo "  -t, --test        Run tests after build"
    echo "  -h, --help        Show this help"
}

CLEAN=0
BUILD_DEPS=0
CLEAN_DEPS=0
RUN_TESTS=0

while [[ $# -gt 0 ]]; do
    case $1 in
        -c|--clean)
            CLEAN=1
            shift
            ;;
        -d|--deps)
            BUILD_DEPS=1
            shift
            ;;
        --clean-deps)
            CLEAN_DEPS=1
            BUILD_DEPS=1
            shift
            ;;
        -j)
            JOBS="$2"
            shift 2
            ;;
        -t|--test)
            RUN_TESTS=1
            shift
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

# Check if dependencies exist
YAMLCPP_OK=0
ICEORYX_OK=0
CYCLONEDDS_OK=0
CYCLONEDDS_CXX_OK=0

if [[ -f "${THIRD_PARTY}/yaml-cpp/lib/libyaml-cpp.a" ]]; then
    YAMLCPP_OK=1
fi

if [[ -f "${THIRD_PARTY}/iceoryx/lib/libiceoryx_posh.a" ]]; then
    ICEORYX_OK=1
fi

if [[ -f "${THIRD_PARTY}/cyclonedds-${CYCLONEDDS_VERSION}/lib/libddsc.a" ]]; then
    CYCLONEDDS_OK=1
fi

if [[ -f "${THIRD_PARTY}/cyclonedds-cxx-${CYCLONEDDS_VERSION}/lib/libddscxx.a" ]]; then
    CYCLONEDDS_CXX_OK=1
fi

# Build dependencies if requested or missing
DEPS_MISSING=0
if [[ $YAMLCPP_OK -eq 0 ]] || [[ $ICEORYX_OK -eq 0 ]] || [[ $CYCLONEDDS_OK -eq 0 ]] || [[ $CYCLONEDDS_CXX_OK -eq 0 ]]; then
    DEPS_MISSING=1
fi

if [[ $BUILD_DEPS -eq 1 ]] || [[ $DEPS_MISSING -eq 1 ]]; then
    if [[ $DEPS_MISSING -eq 1 ]]; then
        echo "==> Dependencies not found, building..."
    fi

    DEPS_ARGS="-j ${JOBS}"
    if [[ $CLEAN_DEPS -eq 1 ]]; then
        DEPS_ARGS="${DEPS_ARGS} --clean"
    fi

    "${PROJECT_ROOT}/scripts/build_deps.sh" ${DEPS_ARGS}
fi

if [[ $CLEAN -eq 1 ]]; then
    echo "==> Cleaning build directory..."
    rm -rf "${BUILD_DIR}"
fi

echo "==> Configuring..."
mkdir -p "${BUILD_DIR}"
cmake -S "${PROJECT_ROOT}" -B "${BUILD_DIR}"

echo "==> Building with ${JOBS} jobs..."
cmake --build "${BUILD_DIR}" -j "${JOBS}"

echo "==> Build complete!"
echo "    CLI: ${BUILD_DIR}/cli/cddsctl"

if [[ $RUN_TESTS -eq 1 ]]; then
    echo "==> Running tests..."
    "${BUILD_DIR}/tests/cddsctl_tests"
fi
