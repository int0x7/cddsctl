#!/bin/bash
# 自动测试所有 IDL 示例
# 用法: ./test_idl_examples.sh

set -e

echo "=== cddsctl IDL 示例自动测试 ==="
echo ""

# 颜色输出
RED='\033[0;31m'
GREEN='\033[0;32m'
NC='\033[0m' # No Color

# 测试结果统计
PASSED=0
FAILED=0

# 测试函数
test_idl() {
    local name=$1
    local publisher=$2
    local topic=$3
    local timeout_sec=${4:-5}

    echo -n "测试 $name ... "

    # 启动 publisher
    CYCLONEDDS_URI="/etc/cyclonedds/cyclonedds_shm.xml" timeout $timeout_sec ./build/examples/$publisher --topic $topic --rate 2 >/dev/null 2>&1 &
    local pub_pid=$!

    # 等待 publisher 启动
    sleep 1

    # 运行 cddsctl echo 并捕获输出
    local output
    if output=$(timeout 3 ./build/cli/cddsctl echo $topic -n 1 2>/dev/null); then
        if echo "$output" | grep -q "^---"; then
            echo -e "${GREEN}通过${NC}"
            ((PASSED++))
        else
            echo -e "${RED}失败${NC} (无有效输出)"
            ((FAILED++))
        fi
    else
        echo -e "${RED}失败${NC} (echo 命令失败)"
        ((FAILED++))
    fi

    # 清理 publisher
    kill $pub_pid 2>/dev/null || true
    wait $pub_pid 2>/dev/null || true

    # 等待 DDS 资源释放
    sleep 1
}

# 检查二进制文件是否存在
if [ ! -f ./build/cli/cddsctl ]; then
    echo "错误: cddsctl 未编译. 请先运行: cmake --build build"
    exit 1
fi

# 运行所有测试
test_idl "NestedStruct" "nested_struct_publisher" "/test/nested"
test_idl "ArraysAndSequences" "array_publisher" "/test/array"
test_idl "Enumeration" "enum_publisher" "/test/enum"
test_idl "VariousTypes" "various_types_publisher" "/test/types"
test_idl "UnionType" "union_publisher" "/test/union"
test_idl "AdvancedFeatures" "advanced_publisher" "/test/advanced"

echo ""
echo "=== 测试结果 ==="
echo -e "通过: ${GREEN}$PASSED${NC}"
echo -e "失败: ${RED}$FAILED${NC}"
echo "总计: $((PASSED + FAILED))"

if [ $FAILED -eq 0 ]; then
    echo -e "${GREEN}所有测试通过!${NC}"
    exit 0
else
    echo -e "${RED}部分测试失败!${NC}"
    exit 1
fi
