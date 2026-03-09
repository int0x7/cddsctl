#!/bin/bash
#
# Integration test for IDL examples with cddsctl
#
# This script tests that:
# 1. All IDL publishers can be built and run
# 2. cddsctl echo correctly displays various IDL types
# 3. cddsctl record correctly records various IDL types to MCAP
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "${SCRIPT_DIR}/../.." && pwd)"
BUILD_DIR="${PROJECT_ROOT}/build"
TEST_OUTPUT_DIR="${PROJECT_ROOT}/build/tests/integration"
DOMAIN_BASE=100

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

log_info() {
    echo -e "${GREEN}[INFO]${NC} $1"
}

log_warn() {
    echo -e "${YELLOW}[WARN]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Check if required binaries exist
check_binaries() {
    local missing=()

    for binary in cddsctl nested_struct_publisher array_publisher various_types_publisher enum_publisher union_publisher advanced_publisher; do
        if [[ ! -f "${BUILD_DIR}/examples/${binary}" ]]; then
            missing+=("${binary}")
        fi
    done

    if [[ ${#missing[@]} -gt 0 ]]; then
        log_error "Missing binaries: ${missing[*]}"
        log_info "Please build the project first: cmake --build build"
        exit 1
    fi
}

# Run a publisher in background and return its PID
start_publisher() {
    local publisher="$1"
    local topic="$2"
    local domain="$3"

    log_info "Starting ${publisher} on topic ${topic} (domain ${domain})"

    # Start publisher in background
    "${BUILD_DIR}/examples/${publisher}" \
        --topic "${topic}" \
        --domain "${domain}" \
        --rate 10 \
        > /dev/null 2>&1 &

    echo $!
}

# Test cddsctl echo with timeout
test_echo() {
    local topic="$1"
    local domain="$2"
    local name="$3"

    log_info "Testing cddsctl echo for ${name}..."

    # Run echo with timeout and count lines
    local output
    if output=$(timeout 5 "${BUILD_DIR}/cli/cddsctl" echo "${topic}" -n 3 --domain "${domain}" 2>&1); then
        local line_count
        line_count=$(echo "${output}" | wc -l)

        if [[ ${line_count} -gt 0 ]]; then
            log_info "✓ ${name}: Received ${line_count} lines of output"
            return 0
        else
            log_error "✗ ${name}: No output received"
            return 1
        fi
    else
        log_error "✗ ${name}: echo command failed or timed out"
        return 1
    fi
}

# Test cddsctl record
test_record() {
    local topic="$1"
    local domain="$2"
    local name="$3"

    log_info "Testing cddsctl record for ${name}..."

    local output_file="${TEST_OUTPUT_DIR}/${name}_test.mcap"

    # Run record with timeout
    if timeout 8 "${BUILD_DIR}/cli/cddsctl" record "${topic}" \
        -O "${output_file%.mcap}" \
        --duration=5 \
        --domain "${domain}" 2>&1; then

        # Check if file was created and has content
        if [[ -f "${output_file}" ]]; then
            local file_size
            file_size=$(stat -f%z "${output_file}" 2>/dev/null || stat -c%s "${output_file}" 2>/dev/null)

            if [[ ${file_size} -gt 100 ]]; then
                log_info "✓ ${name}: Recorded ${file_size} bytes to ${output_file}"
                return 0
            else
                log_error "✗ ${name}: MCAP file too small (${file_size} bytes)"
                return 1
            fi
        else
            log_error "✗ ${name}: MCAP file not created"
            return 1
        fi
    else
        log_error "✗ ${name}: record command failed"
        return 1
    fi
}

# Run integration test for a specific IDL type
run_idl_test() {
    local publisher="$1"
    local topic="$2"
    local domain="$3"
    local name="$4"

    log_info "=========================================="
    log_info "Testing ${name}"
    log_info "=========================================="

    local pid
    pid=$(start_publisher "${publisher}" "${topic}" "${domain}")

    # Wait for publisher to initialize
    sleep 2

    local result=0

    # Test echo
    if ! test_echo "${topic}" "${domain}" "${name}"; then
        result=1
    fi

    # Test record (only if echo succeeded)
    if [[ ${result} -eq 0 ]]; then
        if ! test_record "${topic}" "${domain}" "${name}"; then
            result=1
        fi
    fi

    # Cleanup publisher
    kill "${pid}" 2>/dev/null || true
    wait "${pid}" 2>/dev/null || true

    return ${result}
}

# Main test execution
main() {
    log_info "=========================================="
    log_info "IDL Integration Test Suite"
    log_info "=========================================="

    # Create output directory
    mkdir -p "${TEST_OUTPUT_DIR}"

    # Check binaries
    check_binaries

    local total_tests=0
    local passed_tests=0

    # Define test cases
    declare -a tests=(
        "nested_struct_publisher:/test/nested_struct:$((DOMAIN_BASE)):NestedStruct"
        "array_publisher:/test/arrays:$((DOMAIN_BASE + 1)):ArraysAndSequences"
        "various_types_publisher:/test/various_types:$((DOMAIN_BASE + 2)):VariousTypes"
        "enum_publisher:/test/enums:$((DOMAIN_BASE + 3)):Enumeration"
        "union_publisher:/test/unions:$((DOMAIN_BASE + 4)):UnionType"
        "advanced_publisher:/test/advanced:$((DOMAIN_BASE + 5)):AdvancedFeatures"
    )

    for test in "${tests[@]}"; do
        IFS=':' read -r publisher topic domain name <<< "${test}"

        total_tests=$((total_tests + 1))

        if run_idl_test "${publisher}" "${topic}" "${domain}" "${name}"; then
            passed_tests=$((passed_tests + 1))
        fi

        # Brief pause between tests
        sleep 1
    done

    # Summary
    log_info "=========================================="
    log_info "Test Summary"
    log_info "=========================================="
    log_info "Total: ${total_tests}"
    log_info "Passed: ${passed_tests}"
    log_info "Failed: $((total_tests - passed_tests))"

    if [[ ${passed_tests} -eq ${total_tests} ]]; then
        log_info "All tests passed!"
        exit 0
    else
        log_error "Some tests failed!"
        exit 1
    fi
}

# Run main if executed directly
if [[ "${BASH_SOURCE[0]}" == "${0}" ]]; then
    main "$@"
fi
