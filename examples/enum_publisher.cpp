/**
 * @file enum_publisher.cpp
 * @brief Publisher for Enumeration IDL demonstrating enum types
 */

#include <chrono>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <thread>

#include <dds/dds.hpp>
#include "Enumeration.hpp"

namespace {
volatile std::sig_atomic_t g_running = 1;

void signal_handler(int) { g_running = 0; }

void print_usage(const char* prog_name) {
    std::cout << "Usage: " << prog_name << " [OPTIONS]\n"
              << "\n"
              << "Options:\n"
              << "  --topic <name>    Topic name (default: /robot/status)\n"
              << "  --rate <hz>       Publishing rate in Hz (default: 2)\n"
              << "  --domain <id>     DDS domain ID (default: 0)\n"
              << "  --help            Show this help message\n"
              << std::endl;
}

struct Options {
    std::string topic_name = "/robot/status";
    double rate = 2.0;
    int domain_id = 0;
};

bool parse_args(int argc, char* argv[], Options& opts) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--help" || arg == "-h") {
            print_usage(argv[0]);
            return false;
        } else if (arg == "--topic" && i + 1 < argc) {
            opts.topic_name = argv[++i];
        } else if (arg == "--rate" && i + 1 < argc) {
            opts.rate = std::atof(argv[++i]);
        } else if (arg == "--domain" && i + 1 < argc) {
            opts.domain_id = std::atoi(argv[++i]);
        }
    }
    return true;
}

const char* status_to_string(demo::enums::Status s) {
    switch (s) {
        case demo::enums::Status::STATUS_UNKNOWN: return "UNKNOWN";
        case demo::enums::Status::STATUS_OK: return "OK";
        case demo::enums::Status::STATUS_WARNING: return "WARNING";
        case demo::enums::Status::STATUS_ERROR: return "ERROR";
        case demo::enums::Status::STATUS_FATAL: return "FATAL";
    }
    return "INVALID";
}

const char* mode_to_string(demo::enums::OperationMode m) {
    switch (m) {
        case demo::enums::OperationMode::MODE_IDLE: return "IDLE";
        case demo::enums::OperationMode::MODE_INITIALIZING: return "INITIALIZING";
        case demo::enums::OperationMode::MODE_STANDBY: return "STANDBY";
        case demo::enums::OperationMode::MODE_ACTIVE: return "ACTIVE";
        case demo::enums::OperationMode::MODE_SHUTDOWN: return "SHUTDOWN";
        case demo::enums::OperationMode::MODE_EMERGENCY_STOP: return "EMERGENCY_STOP";
    }
    return "INVALID";
}
}  // namespace

int main(int argc, char* argv[]) {
    Options opts;
    if (!parse_args(argc, argv, opts)) return 1;

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    try {
        dds::domain::DomainParticipant participant(opts.domain_id);
        dds::topic::Topic<demo::enums::RobotStatus> topic(participant, opts.topic_name);
        dds::pub::Publisher publisher(participant);
        dds::pub::DataWriter<demo::enums::RobotStatus> writer(publisher, topic);

        std::cout << "Publishing RobotStatus to: " << opts.topic_name << std::endl;

        demo::enums::RobotStatus status;
        auto sleep_duration = std::chrono::duration<double>(1.0 / opts.rate);
        int count = 0;

        // Define enum values for cycling
        const demo::enums::Status statuses[] = {
            demo::enums::Status::STATUS_OK,
            demo::enums::Status::STATUS_WARNING,
            demo::enums::Status::STATUS_OK,
            demo::enums::Status::STATUS_ERROR,
            demo::enums::Status::STATUS_OK
        };

        const demo::enums::OperationMode modes[] = {
            demo::enums::OperationMode::MODE_IDLE,
            demo::enums::OperationMode::MODE_INITIALIZING,
            demo::enums::OperationMode::MODE_STANDBY,
            demo::enums::OperationMode::MODE_ACTIVE,
            demo::enums::OperationMode::MODE_SHUTDOWN
        };

        while (g_running) {
            auto now = std::chrono::system_clock::now().time_since_epoch();
            long long sec = std::chrono::duration_cast<std::chrono::seconds>(now).count();

            // System status with enums
            status.system().timestamp(sec);
            status.system().overall_status(statuses[count % 5]);
            status.system().current_mode(modes[count % 5]);
            status.system().reference_frame(demo::enums::CoordinateFrame::FRAME_WORLD);
            status.system().signal_quality(static_cast<demo::enums::DataQuality>(75 + (count % 25)));
            status.system().is_homed(count % 3 == 0);
            status.system().motors_enabled(count % 4 != 0);

            // Joint info with joint type enum
            status.joints().resize(6);
            const demo::enums::JointType joint_types[] = {
                demo::enums::JointType::JOINT_REVOLUTE,
                demo::enums::JointType::JOINT_REVOLUTE,
                demo::enums::JointType::JOINT_PRISMATIC,
                demo::enums::JointType::JOINT_REVOLUTE,
                demo::enums::JointType::JOINT_REVOLUTE,
                demo::enums::JointType::JOINT_FIXED
            };

            for (int i = 0; i < 6; ++i) {
                status.joints()[i].name("joint_" + std::to_string(i));
                status.joints()[i].type(joint_types[i]);
                status.joints()[i].status(statuses[(count + i) % 5]);
                status.joints()[i].position(i * 0.1);
                status.joints()[i].limit_min(-3.14);
                status.joints()[i].limit_max(3.14);
            }

            status.status_message("System operating normally - cycle " + std::to_string(count));

            writer.write(status);

            std::cout << "Published RobotStatus #" << count
                      << " [status: " << status_to_string(status.system().overall_status())
                      << ", mode: " << mode_to_string(status.system().current_mode())
                      << ", joints: " << status.joints().size() << "]\r" << std::flush;

            ++count;
            std::this_thread::sleep_for(sleep_duration);
        }

        std::cout << "\nPublisher stopped. Published " << count << " messages." << std::endl;
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
