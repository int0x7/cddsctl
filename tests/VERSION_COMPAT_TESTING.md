# Version Compatibility Testing

This document describes how to test compatibility between cddsctl tool built with different CycloneDDS versions and test publishers built with other versions.

## Overview

The version compatibility test framework verifies that:
1. cddsctl built with CycloneDDS version X can communicate with publishers built with version Y
2. Core DDS functionality (discovery, data exchange) works across minor version differences

## Supported Versions

Currently supported CycloneDDS versions for testing:
- 0.10.2
- 0.10.3
- 0.10.5

## Test Scripts

### Python Test Runner (Recommended)

**File:** `tests/test_version_compat.py`

The Python script provides a flexible way to run compatibility tests with proper logging and JSON output.

#### Usage

```bash
# List supported versions
python3 tests/test_version_compat.py --list-versions

# Test specific combination (cddsctl 0.10.2 with publisher 0.10.5)
python3 tests/test_version_compat.py 0.10.2 0.10.5

# Test all version combinations
python3 tests/test_version_compat.py --test-all

# Save results to JSON
python3 tests/test_version_compat.py --test-all --json-output results.json

# Use custom build directory
python3 tests/test_version_compat.py 0.10.2 0.10.5 --build-dir /tmp/compat_test
```

#### What It Tests

1. **Echo Command Test**
   - Starts test publisher (version Y) publishing messages
   - Runs `cddsctl echo` (version X) to receive messages
   - Verifies at least 3 messages are received correctly

2. **List Command Test**
   - Starts test publisher (version Y)
   - Runs `cddsctl list` (version X) to discover topics
   - Verifies the test topic appears in the list

### Bash Test Script

**File:** `tests/version_compat_test.sh`

A simpler bash-based test script for environments without Python.

#### Usage

```bash
# Test specific combination
./tests/version_compat_test.sh 0.10.2 0.10.5
```

## How It Works

### Build Process

1. **Dependencies Build**
   - Each CycloneDDS version requires its own set of dependencies
   - Dependencies are built in isolated directories:
     ```
     build_compat_test/
     ├── deps_0.10.2/
     │   ├── cyclonedds/
     │   ├── cyclonedds-cxx/
     │   ├── iceoryx/
     │   └── yaml-cpp/
     └── deps_0.10.5/
         └── ...
     ```

2. **Tool Build**
   - cddsctl is built with specified CycloneDDS version
   - Output: `build_compat_test/tool_{version}/cli/cddsctl`

3. **Publisher Build**
   - Test publisher is built with specified CycloneDDS version
   - Uses IDL compiler from that version
   - Output: `build_compat_test/pub_{version}/test_publisher`

### Test Execution

Tests run in isolated DDS domains (42, 43, etc.) to avoid interfering with other DDS applications.

## CI Integration

### GitHub Actions Example

```yaml
name: Version Compatibility Tests

on:
  push:
    branches: [ main ]
  pull_request:
    branches: [ main ]
  schedule:
    # Run weekly
    - cron: '0 0 * * 0'

jobs:
  compat-test:
    runs-on: ubuntu-22.04
    strategy:
      matrix:
        tool_version: ['0.10.2', '0.10.5']
        pub_version: ['0.10.2', '0.10.5']

    steps:
    - uses: actions/checkout@v4

    - name: Install dependencies
      run: |
        sudo apt-get update
        sudo apt-get install -y cmake g++ python3 git

    - name: Run compatibility test
      run: |
        python3 tests/test_version_compat.py \
          ${{ matrix.tool_version }} \
          ${{ matrix.pub_version }} \
          --json-output results.json

    - name: Upload results
      uses: actions/upload-artifact@v4
      if: always()
      with:
        name: compat-results-${{ matrix.tool_version }}-${{ matrix.pub_version }}
        path: results.json
```

### Full Matrix Test

```yaml
    - name: Test all version combinations
      run: |
        python3 tests/test_version_compat.py --test-all \
          --json-output all_results.json

    - name: Check results
      run: |
        failed=$(jq '[.[] | select(.passed == false)] | length' all_results.json)
        if [ "$failed" -gt 0 ]; then
          echo "$failed test(s) failed"
          jq '.[] | select(.passed == false)' all_results.json
          exit 1
        fi
```

## Expected Results

### Same Version (Expected: PASS)
- cddsctl 0.10.2 + publisher 0.10.2 ✓
- cddsctl 0.10.5 + publisher 0.10.5 ✓

### Cross Version (Depends on ABI compatibility)
- cddsctl 0.10.2 + publisher 0.10.3 (likely ✓ - patch version)
- cddsctl 0.10.2 + publisher 0.10.5 (check wire protocol compatibility)
- cddsctl 0.10.5 + publisher 0.10.2 (check backward compatibility)

## Troubleshooting

### Build Failures

**Problem:** CMake can't find dependencies
```
Solution: Delete build_compat_test directory and retry
rm -rf build_compat_test
python3 tests/test_version_compat.py 0.10.2 0.10.5
```

**Problem:** Git clone fails
```
Solution: Check network connectivity or use local mirrors
```

### Test Failures

**Problem:** Tests timeout
```
Solution: Increase timeout in test script or check DDS network configuration
export CYCLONEDDS_URI="<General><NetworkInterfaceAddress>lo</NetworkInterfaceAddress></General>"
```

**Problem:** Messages not received
```
Solution: Check firewall settings and DDS multicast configuration
# Use loopback only for testing
export CYCLONEDDS_URI="<General><AllowMulticast>spdp</AllowMulticast></General>"
```

### Debug Mode

Enable verbose output in Python script:
```python
# Add to test_version_compat.py
import logging
logging.basicConfig(level=logging.DEBUG)
```

## Adding New Versions

To add support for a new CycloneDDS version (e.g., 0.10.6):

1. Update `SUPPORTED_VERSIONS` in `test_version_compat.py`:
```python
SUPPORTED_VERSIONS = ["0.10.2", "0.10.3", "0.10.5", "0.10.6"]
```

2. Check if other dependency versions need updates

3. Run tests:
```bash
python3 tests/test_version_compat.py --test-all
```

## Notes

- First run will take significant time (downloads and builds all dependencies)
- Subsequent runs are faster (uses cached builds)
- Tests use isolated DDS domains to avoid interference
- Each version combination requires ~500MB disk space for dependencies
