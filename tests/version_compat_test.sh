#!/bin/bash
#
# Version Compatibility Test for cddsctl
#
# This script tests compatibility between cddsctl tool built with one
# CycloneDDS version and test publisher built with another version.
#
# Usage:
#   ./version_compat_test.sh <tool_version> <publisher_version>
#
# Example:
#   ./version_compat_test.sh 0.10.2 0.10.5
#   (Tests cddsctl built with 0.10.2 against publisher built with 0.10.5)
#

set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
TESTS_DIR="${PROJECT_ROOT}/tests"
BUILD_BASE="${PROJECT_ROOT}/build_compat_test"
CONFIG_FILE="${TESTS_DIR}/version_compat_config.yaml"

# Helper function to read YAML values using Python
yaml_get() {
    local key="$1"
    python3 -c "
import yaml
import sys
try:
    with open('$CONFIG_FILE', 'r') as f:
        config = yaml.safe_load(f)
    keys = '$key'.split('.')
    value = config
    for k in keys:
        value = value[k]
    if isinstance(value, list):
        print(' '.join(str(x) for x in value))
    else:
        print(value)
except Exception as e:
    sys.stderr.write(f'Error reading config: {e}\\n')
    sys.exit(1)
"
}

# Load versions from config file
if [[ ! -f "$CONFIG_FILE" ]]; then
    echo "Error: Configuration file not found: $CONFIG_FILE"
    exit 2
fi

# Check if PyYAML is available
if ! python3 -c "import yaml" 2>/dev/null; then
    echo "Error: PyYAML is required. Install it with: pip3 install pyyaml"
    exit 2
fi

SUPPORTED_VERSIONS=$(yaml_get "cyclonedds_versions")
ICEORYX_VERSION=$(yaml_get "dependency_versions.iceoryx")
YAMLCPP_VERSION=$(yaml_get "dependency_versions.yaml_cpp")
MIN_VERSION=$(yaml_get "notes.min_version")
VERSION_REASON=$(yaml_get "notes.reason")

# Parse arguments
TOOL_VERSION="${1:-}"
PUB_VERSION="${2:-}"

if [[ -z "$TOOL_VERSION" || -z "$PUB_VERSION" ]]; then
    echo "Usage: $0 <tool_version> <publisher_version>"
    echo ""
    echo "Example:"
    echo "  $0 0.10.2 0.10.5"
    echo "  (Tests cddsctl 0.10.2 against publisher 0.10.5)"
    echo ""
    echo "Supported versions:"
    for v in $SUPPORTED_VERSIONS; do
        echo "  - $v"
    done
    echo ""
    echo "Notes:"
    echo "  - 0.9.x is NOT supported (cddsctl requires dds_typeinfo_t API from 0.10.1+)"
    echo "  - 11.0.0 requires cddsctl built with compatible iceoryx version"
    exit 1
fi

echo "=========================================="
echo "Version Compatibility Test"
echo "=========================================="
echo "cddsctl tool version:    ${TOOL_VERSION}"
echo "Test publisher version:  ${PUB_VERSION}"
echo "=========================================="

# Create build directories
TOOL_BUILD="${BUILD_BASE}/tool_${TOOL_VERSION}"
PUB_BUILD="${BUILD_BASE}/pub_${PUB_VERSION}"
DEPS_TOOL="${BUILD_BASE}/deps_${TOOL_VERSION}"
DEPS_PUB="${BUILD_BASE}/deps_${PUB_VERSION}"

mkdir -p "$TOOL_BUILD" "$PUB_BUILD" "$DEPS_TOOL" "$DEPS_PUB"

# Function to build dependencies for a specific version
build_deps_for_version() {
    local version="$1"
    local install_prefix="$2"
    local build_dir="$3"

    echo ""
    echo "==> Building dependencies for CycloneDDS ${version}..."

    mkdir -p "$build_dir"
    cd "$build_dir"

    # Build iceoryx
    if [[ ! -f "${install_prefix}/iceoryx/lib/libiceoryx_posh.a" ]]; then
        echo "    Building iceoryx..."
        if [[ ! -d "iceoryx" ]]; then
            git clone --depth 1 --branch v${ICEORYX_VERSION} \
                https://github.com/eclipse-iceoryx/iceoryx.git
        fi
        cmake -S iceoryx/iceoryx_meta -B iceoryx_build \
            -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_INSTALL_PREFIX="${install_prefix}/iceoryx" \
            -DBUILD_SHARED_LIBS=OFF \
            -DROUDI_ENVIRONMENT=ON \
            -DEXAMPLES=OFF \
            -DBINDING_C=ON \
            -DINTROSPECTION=OFF \
            -DBUILD_TEST=OFF 2>&1 | tail -5
        cmake --build iceoryx_build -j$(nproc) 2>&1 | tail -10
        cmake --install iceoryx_build 2>&1 | tail -5
    fi

    # Build yaml-cpp
    if [[ ! -f "${install_prefix}/yaml-cpp/lib/libyaml-cpp.a" ]]; then
        echo "    Building yaml-cpp..."
        if [[ ! -d "yaml-cpp" ]]; then
            git clone --depth 1 --branch ${YAMLCPP_VERSION} \
                https://github.com/jbeder/yaml-cpp.git
        fi
        cmake -S yaml-cpp -B yaml-cpp_build \
            -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_INSTALL_PREFIX="${install_prefix}/yaml-cpp" \
            -DBUILD_SHARED_LIBS=OFF \
            -DYAML_CPP_BUILD_TESTS=OFF \
            -DYAML_CPP_BUILD_TOOLS=OFF \
            -DCMAKE_POSITION_INDEPENDENT_CODE=ON 2>&1 | tail -5
        cmake --build yaml-cpp_build -j$(nproc) 2>&1 | tail -5
        cmake --install yaml-cpp_build 2>&1 | tail -5
    fi

    # Build CycloneDDS
    if [[ ! -f "${install_prefix}/cyclonedds/lib/libddsc.a" ]]; then
        echo "    Building CycloneDDS ${version}..."
        if [[ ! -d "cyclonedds" ]]; then
            git clone --depth 1 --branch "${version}" \
                https://github.com/eclipse-cyclonedds/cyclonedds.git
        fi
        cmake -S cyclonedds -B cyclonedds_build \
            -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_INSTALL_PREFIX="${install_prefix}/cyclonedds" \
            -DCMAKE_PREFIX_PATH="${install_prefix}/iceoryx" \
            -DBUILD_SHARED_LIBS=OFF \
            -DENABLE_SHM=ON \
            -DENABLE_SECURITY=OFF \
            -DENABLE_SSL=OFF \
            -DBUILD_IDLC=ON \
            -DBUILD_DDSPERF=OFF \
            -DBUILD_EXAMPLES=OFF 2>&1 | tail -5
        cmake --build cyclonedds_build -j$(nproc) 2>&1 | tail -10
        cmake --install cyclonedds_build 2>&1 | tail -5
    fi

    # Build CycloneDDS-CXX
    if [[ ! -f "${install_prefix}/cyclonedds-cxx/lib/libddscxx.a" ]]; then
        echo "    Building CycloneDDS-CXX ${version}..."
        if [[ ! -d "cyclonedds-cxx" ]]; then
            git clone --depth 1 --branch "${version}" \
                https://github.com/eclipse-cyclonedds/cyclonedds-cxx.git
        fi
        cmake -S cyclonedds-cxx -B cyclonedds-cxx_build \
            -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_INSTALL_PREFIX="${install_prefix}/cyclonedds-cxx" \
            -DCMAKE_PREFIX_PATH="${install_prefix}/iceoryx;${install_prefix}/cyclonedds" \
            -DBUILD_SHARED_LIBS=OFF \
            -DBUILD_EXAMPLES=OFF \
            -DBUILD_TESTING=OFF 2>&1 | tail -5
        cmake --build cyclonedds-cxx_build -j$(nproc) 2>&1 | tail -10
        cmake --install cyclonedds-cxx_build 2>&1 | tail -5
    fi

    echo "    Dependencies built for ${version}"
}

# Function to build cddsctl tool with specific version
build_tool() {
    local version="$1"
    local install_prefix="$2"
    local build_dir="$3"

    echo ""
    echo "==> Building cddsctl tool with CycloneDDS ${version}..."

    mkdir -p "$build_dir"
    cd "$build_dir"

    cmake -S "$PROJECT_ROOT" -B . \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_PREFIX_PATH="${install_prefix}/yaml-cpp;${install_prefix}/iceoryx;${install_prefix}/cyclonedds;${install_prefix}/cyclonedds-cxx" \
        -DBUILD_TESTING=OFF \
        -DBUILD_EXAMPLES=OFF 2>&1 | tail -10

    cmake --build . -j$(nproc) --target cddsctl 2>&1 | tail -10

    echo "    cddsctl built: ${build_dir}/cli/cddsctl"
}

# Function to build test publisher with specific version
build_publisher() {
    local version="$1"
    local install_prefix="$2"
    local build_dir="$3"

    echo ""
    echo "==> Building test publisher with CycloneDDS ${version}..."

    mkdir -p "$build_dir"
    cd "$build_dir"

    # Generate types from IDL
    "${install_prefix}/cyclonedds/bin/idlc" -x cxx \
        "${PROJECT_ROOT}/examples/TestTypes.idl" \
        -o . 2>&1 | tail -5

    # Compile publisher
    g++ -std=c++17 -O2 \
        -I "${PROJECT_ROOT}/examples" \
        -I "${install_prefix}/cyclonedds/include" \
        -I "${install_prefix}/cyclonedds-cxx/include" \
        -I "${install_prefix}/iceoryx/include" \
        "${PROJECT_ROOT}/examples/test_publisher.cpp" \
        TestTypes.cpp \
        -L "${install_prefix}/cyclonedds-cxx/lib" \
        -L "${install_prefix}/cyclonedds/lib" \
        -L "${install_prefix}/iceoryx/lib" \
        -lddscxx -lddsc \
        -liceoryx_posh -liceoryx_hoofs -liceoryx_binding_c \
        -lpthread \
        -o test_publisher 2>&1 | tail -10

    echo "    Publisher built: ${build_dir}/test_publisher"
}

# Function to run compatibility test
run_compat_test() {
    local tool_build="$1"
    local pub_build="$2"
    local test_name="$3"

    echo ""
    echo "=========================================="
    echo "Running: ${test_name}"
    echo "=========================================="

    local cddsctl="${tool_build}/cli/cddsctl"
    local publisher="${pub_build}/test_publisher"
    local test_topic="/compat_test"
    local domain="42"  # Use non-default domain to avoid conflicts

    # Check binaries exist
    if [[ ! -f "$cddsctl" ]]; then
        echo "ERROR: cddsctl not found at ${cddsctl}"
        return 1
    fi
    if [[ ! -f "$publisher" ]]; then
        echo "ERROR: test_publisher not found at ${publisher}"
        return 1
    fi

    echo "cddsctl:   ${cddsctl}"
    echo "publisher: ${publisher}"
    echo "topic:     ${test_topic}"

    # Test 1: Basic echo test
    echo ""
    echo "Test 1: echo command (receive 5 messages)"
    echo "----------------------------------------"

    # Start publisher in background
    export CYCLONEDDS_URI="<General><DomainId>${domain}</DomainId></General>"
    "$publisher" --topic "$test_topic" --count 10 --rate 10 --domain "$domain" &
    local pub_pid=$!

    # Wait for publisher to start
    sleep 2

    # Run echo command
    echo "Running: cddsctl echo ${test_topic} -n 5 --domain ${domain}"
    timeout 10 "$cddsctl" echo "$test_topic" -n 5 --domain "$domain" 2>&1 || true

    # Cleanup publisher
    kill $pub_pid 2>/dev/null || true
    wait $pub_pid 2>/dev/null || true
    unset CYCLONEDDS_URI

    # Test 2: List topics test
    echo ""
    echo "Test 2: list command"
    echo "----------------------------------------"

    export CYCLONEDDS_URI="<General><DomainId>${domain}</DomainId></General>"
    "$publisher" --topic "$test_topic" --count 100 --rate 10 --domain "$domain" &
    pub_pid=$!

    sleep 2

    echo "Running: cddsctl list --domain ${domain}"
    timeout 5 "$cddsctl" list --domain "$domain" 2>&1 || true

    kill $pub_pid 2>/dev/null || true
    wait $pub_pid 2>/dev/null || true
    unset CYCLONEDDS_URI

    echo ""
    echo "=========================================="
    echo "Test completed: ${test_name}"
    echo "=========================================="
}

# Main execution
echo ""
echo "Step 1: Build dependencies and cddsctl tool (v${TOOL_VERSION})"
build_deps_for_version "$TOOL_VERSION" "$DEPS_TOOL" "${BUILD_BASE}/build_deps_${TOOL_VERSION}"
build_tool "$TOOL_VERSION" "$DEPS_TOOL" "$TOOL_BUILD"

echo ""
echo "Step 2: Build dependencies and test publisher (v${PUB_VERSION})"
build_deps_for_version "$PUB_VERSION" "$DEPS_PUB" "${BUILD_BASE}/build_deps_${PUB_VERSION}"
build_publisher "$PUB_VERSION" "$DEPS_PUB" "$PUB_BUILD"

echo ""
echo "Step 3: Run compatibility tests"
run_compat_test "$TOOL_BUILD" "$PUB_BUILD" "cddsctl-${TOOL_VERSION} + publisher-${PUB_VERSION}"

echo ""
echo "=========================================="
echo "All compatibility tests completed!"
echo "=========================================="
echo ""
echo "Build artifacts location: ${BUILD_BASE}"
echo "  cddsctl tool:   ${TOOL_BUILD}/cli/cddsctl"
echo "  test publisher: ${PUB_BUILD}/test_publisher"
