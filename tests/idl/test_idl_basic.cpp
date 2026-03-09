/**
 * @file test_idl_basic.cpp
 * @brief Basic tests for IDL generated types
 *
 * Tests struct creation, field access, sequences, unions, and enums
 * from the example IDL files.
 */

#include <catch.hpp>
#include <cstring>

// Include generated IDL headers
#include "NestedStruct.hpp"
#include "ArraysAndSequences.hpp"
#include "VariousTypes.hpp"
#include "Enumeration.hpp"
#include "UnionType.hpp"
#include "AdvancedFeatures.hpp"

using namespace demo::robotics;
using namespace demo::arrays;
using namespace demo::types;
using namespace demo::enums;
using namespace demo::unions;
using namespace demo::advanced;

TEST_CASE("NestedStruct basic operations", "[idl][nested]") {
    SECTION("Vector3 creation and access") {
        Vector3 vec;
        vec.x(1.0);
        vec.y(2.0);
        vec.z(3.0);

        REQUIRE(vec.x() == Approx(1.0));
        REQUIRE(vec.y() == Approx(2.0));
        REQUIRE(vec.z() == Approx(3.0));
    }

    SECTION("Quaternion creation") {
        Quaternion quat;
        quat.x(0.0);
        quat.y(0.0);
        quat.z(0.0);
        quat.w(1.0);

        REQUIRE(quat.w() == Approx(1.0));
    }

    SECTION("Pose nesting") {
        Pose pose;
        pose.position().x(1.0);
        pose.position().y(2.0);
        pose.position().z(3.0);
        pose.orientation().w(1.0);

        REQUIRE(pose.position().x() == Approx(1.0));
        REQUIRE(pose.orientation().w() == Approx(1.0));
    }

    SECTION("RobotState with joint sequence") {
        RobotState state;
        state.timestamp_sec(1234567890);
        state.timestamp_nsec(123456789);
        state.frame_id("base_link");
        state.is_active(true);
        state.status_code(42);

        REQUIRE(state.timestamp_sec() == 1234567890);
        REQUIRE(state.frame_id() == "base_link");

        // Test joint sequence
        state.joints().resize(6);
        REQUIRE(state.joints().size() == 6);

        for (size_t i = 0; i < 6; ++i) {
            state.joints()[i].name("joint_" + std::to_string(i));
            state.joints()[i].position(static_cast<double>(i) * 0.1);
        }

        REQUIRE(state.joints()[0].name() == "joint_0");
        REQUIRE(state.joints()[5].position() == Approx(0.5));
    }
}

TEST_CASE("ArraysAndSequences operations", "[idl][arrays]") {
    SECTION("Fixed array access") {
        FixedArrays arrays;

        // 1D array
        arrays.position_3d()[0] = 1.0;
        arrays.position_3d()[1] = 2.0;
        arrays.position_3d()[2] = 3.0;

        REQUIRE(arrays.position_3d()[0] == Approx(1.0));
        REQUIRE(arrays.position_3d()[2] == Approx(3.0));

        // 2D array
        arrays.transform_3x3()[0][0] = 1.0;
        arrays.transform_3x3()[1][1] = 1.0;
        arrays.transform_3x3()[2][2] = 1.0;

        REQUIRE(arrays.transform_3x3()[0][0] == Approx(1.0));
        REQUIRE(arrays.transform_3x3()[1][1] == Approx(1.0));
    }

    SECTION("Unbounded sequence") {
        UnboundedSequences seqs;

        seqs.double_values().resize(100);
        REQUIRE(seqs.double_values().size() == 100);

        for (size_t i = 0; i < 100; ++i) {
            seqs.double_values()[i] = static_cast<double>(i) * 0.1;
        }

        REQUIRE(seqs.double_values()[50] == Approx(5.0));
    }

    SECTION("Bounded sequence") {
        BoundedSequences seqs;

        seqs.recent_positions().resize(10);
        REQUIRE(seqs.recent_positions().size() == 10);

        seqs.joint_names().resize(3);
        seqs.joint_names()[0] = "shoulder";
        seqs.joint_names()[1] = "elbow";
        seqs.joint_names()[2] = "wrist";

        REQUIRE(seqs.joint_names()[1] == "elbow");
    }

    SECTION("Mixed arrays - sequence of Pose6D structs") {
        MixedArrays mixed;

        // Current pose (fixed array)
        mixed.current_pose()[0] = 1.0;
        mixed.current_pose()[1] = 2.0;
        mixed.current_pose()[5] = 0.5;

        REQUIRE(mixed.current_pose()[5] == Approx(0.5));

        // Pose history (sequence of Pose6D structs)
        mixed.pose_history().resize(5);
        REQUIRE(mixed.pose_history().size() == 5);

        for (size_t i = 0; i < 5; ++i) {
            mixed.pose_history()[i].values()[0] = static_cast<double>(i);
        }

        REQUIRE(mixed.pose_history()[3].values()[0] == Approx(3.0));
    }
}

TEST_CASE("Various primitive types", "[idl][types]") {
    SECTION("Integer types") {
        IntegerTypes ints;

        ints.short_val(-32768);
        ints.long_val(-2147483648);
        ints.longlong_val(-9223372036854775807LL);
        ints.ushort_val(65535);
        ints.ulong_val(4294967295U);
        ints.ulonglong_val(18446744073709551615ULL);
        ints.char_val('A');
        ints.bool_val(true);
        ints.byte_val(255);

        REQUIRE(ints.short_val() == -32768);
        REQUIRE(ints.ushort_val() == 65535);
        REQUIRE(ints.char_val() == 'A');
        REQUIRE(ints.bool_val() == true);
        REQUIRE(ints.byte_val() == 255);
    }

    SECTION("Floating point types") {
        FloatingTypes floats;

        floats.float_val(3.14159f);
        floats.double_val(2.718281828459045);

        REQUIRE(floats.float_val() == Approx(3.14159f));
        REQUIRE(floats.double_val() == Approx(2.718281828459045));
    }

    SECTION("String types") {
        StringTypes strings;

        strings.unbounded_str("This is a long unbounded string");

        REQUIRE(strings.unbounded_str() == "This is a long unbounded string");
    }

    SECTION("DiagnosticData with nested structs") {
        DiagnosticData diag;
        diag.timestamp(1234567890123LL);
        diag.source("test_source");
        diag.is_valid(true);

        diag.integers().short_val(100);
        diag.floats().double_val(1.234);
        diag.strings().unbounded_str("test");

        REQUIRE(diag.timestamp() == 1234567890123LL);
        REQUIRE(diag.integers().short_val() == 100);
        REQUIRE(diag.floats().double_val() == Approx(1.234));
    }
}

TEST_CASE("Enumeration types", "[idl][enums]") {
    SECTION("Status enum values") {
        Status s = Status::STATUS_OK;
        REQUIRE(s == Status::STATUS_OK);

        s = Status::STATUS_ERROR;
        REQUIRE(s == Status::STATUS_ERROR);
    }

    SECTION("OperationMode enum") {
        OperationMode mode = OperationMode::MODE_ACTIVE;
        REQUIRE(mode == OperationMode::MODE_ACTIVE);

        mode = OperationMode::MODE_EMERGENCY_STOP;
        REQUIRE(mode == OperationMode::MODE_EMERGENCY_STOP);
    }

    SECTION("JointType enum") {
        JointType jt = JointType::JOINT_REVOLUTE;
        REQUIRE(jt == JointType::JOINT_REVOLUTE);

        jt = JointType::JOINT_PRISMATIC;
        REQUIRE(jt == JointType::JOINT_PRISMATIC);
    }

    SECTION("DataQuality with explicit values") {
        DataQuality dq = DataQuality::QUALITY_EXCELLENT;
        REQUIRE(dq == DataQuality::QUALITY_EXCELLENT);
        // Note: underlying value access depends on IDL implementation
    }

    SECTION("SystemStatus with enums") {
        SystemStatus sys;
        sys.overall_status(Status::STATUS_WARNING);
        sys.current_mode(OperationMode::MODE_STANDBY);
        sys.reference_frame(CoordinateFrame::FRAME_BASE);
        sys.signal_quality(DataQuality::QUALITY_GOOD);

        REQUIRE(sys.overall_status() == Status::STATUS_WARNING);
        REQUIRE(sys.current_mode() == OperationMode::MODE_STANDBY);
        REQUIRE(sys.reference_frame() == CoordinateFrame::FRAME_BASE);
    }
}

TEST_CASE("Union types", "[idl][unions]") {
    SECTION("DataValue as integer") {
        DataValue val;
        val._d(DataType::TYPE_INTEGER);
        val.int_val(42);

        REQUIRE(val._d() == DataType::TYPE_INTEGER);
        REQUIRE(val.int_val() == 42);
    }

    SECTION("DataValue as float") {
        DataValue val;
        val._d(DataType::TYPE_FLOAT);
        val.float_val(3.14159);

        REQUIRE(val._d() == DataType::TYPE_FLOAT);
        REQUIRE(val.float_val() == Approx(3.14159));
    }

    SECTION("DataValue as string") {
        DataValue val;
        val._d(DataType::TYPE_STRING);
        val.str_val("Hello Union");

        REQUIRE(val._d() == DataType::TYPE_STRING);
        REQUIRE(val.str_val() == "Hello Union");
    }

    SECTION("DataValue as binary") {
        DataValue val;
        val._d(DataType::TYPE_BINARY);
        val.binary_val().resize(4);
        val.binary_val()[0] = 0xDE;
        val.binary_val()[1] = 0xAD;
        val.binary_val()[2] = 0xBE;
        val.binary_val()[3] = 0xEF;

        REQUIRE(val._d() == DataType::TYPE_BINARY);
        REQUIRE(val.binary_val().size() == 4);
        REQUIRE(val.binary_val()[0] == 0xDE);
    }

    SECTION("CommandResult success") {
        CommandResult result;
        result._d(ResultStatus::RESULT_SUCCESS);
        result.success(true);

        REQUIRE(result._d() == ResultStatus::RESULT_SUCCESS);
        REQUIRE(result.success() == true);
    }

    SECTION("CommandResult error with code") {
        CommandResult result;
        result._d(ResultStatus::RESULT_ERROR_CODE);
        result.error_code(404);

        REQUIRE(result._d() == ResultStatus::RESULT_ERROR_CODE);
        REQUIRE(result.error_code() == 404);
    }

    SECTION("SensorReading with union") {
        SensorReading reading;
        reading.timestamp(1234567890);
        reading.sensor_id("sensor_01");
        reading.confidence(0.95f);

        reading.value()._d(DataType::TYPE_FLOAT);
        reading.value().float_val(23.5);

        REQUIRE(reading.sensor_id() == "sensor_01");
        REQUIRE(reading.value()._d() == DataType::TYPE_FLOAT);
        REQUIRE(reading.value().float_val() == Approx(23.5));
    }
}

TEST_CASE("Advanced features", "[idl][advanced]") {
    SECTION("Timestamp") {
        Timestamp ts;
        ts.seconds(1234567890);
        ts.nanoseconds(123456789);

        REQUIRE(ts.seconds() == 1234567890);
        REQUIRE(ts.nanoseconds() == 123456789);
    }

    SECTION("Vector2D and Vector3D") {
        Vector2D v2;
        v2.x(1.0);
        v2.y(2.0);

        Vector3D v3;
        v3.x(1.0);
        v3.y(2.0);
        v3.z(3.0);

        REQUIRE(v2.x() == Approx(1.0));
        REQUIRE(v3.z() == Approx(3.0));
    }

    SECTION("CameraCalibration nested structures") {
        CameraCalibration calib;
        calib.camera_name("camera_01");
        calib.distortion_model("plumb_bob");

        // Set intrinsics
        calib.intrinsics().width(1920);
        calib.intrinsics().height(1080);
        calib.intrinsics().k()[0][0] = 500.0;  // fx
        calib.intrinsics().k()[1][1] = 500.0;  // fy
        calib.intrinsics().k()[0][2] = 960.0;  // cx
        calib.intrinsics().k()[1][2] = 540.0;  // cy
        calib.intrinsics().k()[2][2] = 1.0;

        REQUIRE(calib.intrinsics().width() == 1920);
        REQUIRE(calib.intrinsics().k()[0][0] == Approx(500.0));
    }

    SECTION("KeyValue with VariantValue") {
        KeyValue kv;
        kv.key("temperature");
        kv.value()._d(ValueType::VAL_DOUBLE);
        kv.value().double_val(25.5);

        REQUIRE(kv.key() == "temperature");
        REQUIRE(kv.value()._d() == ValueType::VAL_DOUBLE);
        REQUIRE(kv.value().double_val() == Approx(25.5));
    }

    SECTION("Metadata with key-value sequence") {
        Metadata meta;
        meta.source_system("test_system");

        meta.entries().resize(2);
        meta.entries()[0].key("version");
        meta.entries()[0].value()._d(ValueType::VAL_STRING);
        meta.entries()[0].value().string_val("1.0.0");

        meta.entries()[1].key("count");
        meta.entries()[1].value()._d(ValueType::VAL_INT);
        meta.entries()[1].value().int_val(42);

        REQUIRE(meta.entries().size() == 2);
        REQUIRE(meta.entries()[0].key() == "version");
        REQUIRE(meta.entries()[1].value().int_val() == 42);
    }

    SECTION("DeviceInfo with sub-devices") {
        DeviceInfo device;
        device.device_name("main_controller");
        device.health_score(0.95f);

        device.sub_devices().resize(2);
        device.sub_devices()[0].device_id("sub_01");
        device.sub_devices()[0].device_type("sensor");
        device.sub_devices()[0].is_online(true);

        device.sub_devices()[1].device_id("sub_02");
        device.sub_devices()[1].device_type("actuator");
        device.sub_devices()[1].is_online(false);

        REQUIRE(device.sub_devices().size() == 2);
        REQUIRE(device.sub_devices()[0].is_online() == true);
        REQUIRE(device.sub_devices()[1].is_online() == false);
    }
}
