/**
 * @file nested_struct_publisher.cpp
 * @brief Publisher for NestedStruct IDL demonstrating hierarchical structures
 */

#include <chrono>
#include <cmath>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>

#include <dds/dds.hpp>
#include "NestedStruct.hpp"

namespace {
volatile std::sig_atomic_t g_running = 1;

void signal_handler(int) { g_running = 0; }

void print_usage(const char* prog_name) {
    std::cout << "Usage: " << prog_name << " [OPTIONS]\n"
              << "\n"
              << "Options:\n"
              << "  --topic <name>    Topic name (default: /robot/state)\n"
              << "  --rate <hz>       Publishing rate in Hz (default: 10)\n"
              << "  --domain <id>     DDS domain ID (default: 0)\n"
              << "  --help            Show this help message\n"
              << std::endl;
}

struct Options {
    std::string topic_name = "/robot/state";
    double rate = 10.0;
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
}  // namespace

int main(int argc, char* argv[]) {
    Options opts;
    if (!parse_args(argc, argv, opts)) return 1;

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    try {
        dds::domain::DomainParticipant participant(opts.domain_id);
        dds::topic::Topic<demo::robotics::RobotState> topic(participant, opts.topic_name);
        dds::pub::Publisher publisher(participant);
        dds::pub::DataWriter<demo::robotics::RobotState> writer(publisher, topic);

        std::cout << "Publishing RobotState to: " << opts.topic_name << std::endl;

        demo::robotics::RobotState state;
        auto sleep_duration = std::chrono::duration<double>(1.0 / opts.rate);
        int count = 0;

        while (g_running) {
            auto now = std::chrono::system_clock::now().time_since_epoch();
            auto sec = std::chrono::duration_cast<std::chrono::seconds>(now);
            auto nsec = std::chrono::duration_cast<std::chrono::nanoseconds>(now - sec);

            // Fill header
            state.timestamp_sec(sec.count());
            state.timestamp_nsec(static_cast<uint32_t>(nsec.count()));
            state.frame_id("base_link");
            state.is_active(true);
            state.status_code(static_cast<uint8_t>(count % 256));

            // Fill base pose (nested structures)
            state.base_pose().position().x(std::sin(count * 0.1));
            state.base_pose().position().y(std::cos(count * 0.1));
            state.base_pose().position().z(0.0);
            state.base_pose().orientation().x(0.0);
            state.base_pose().orientation().y(0.0);
            state.base_pose().orientation().z(std::sin(count * 0.05));
            state.base_pose().orientation().w(std::cos(count * 0.05));

            // Fill twist
            state.base_twist().linear().x(std::cos(count * 0.1));
            state.base_twist().linear().y(-std::sin(count * 0.1));
            state.base_twist().linear().z(0.0);
            state.base_twist().angular().x(0.0);
            state.base_twist().angular().y(0.0);
            state.base_twist().angular().z(0.1);

            // Fill wrench
            state.base_wrench().force().x(10.0 * std::sin(count * 0.2));
            state.base_wrench().force().y(10.0 * std::cos(count * 0.2));
            state.base_wrench().force().z(5.0);
            state.base_wrench().torque().x(0.1);
            state.base_wrench().torque().y(0.2);
            state.base_wrench().torque().z(0.3);

            // Fill joints (dynamic sequence)
            state.joints().resize(6);
            for (int i = 0; i < 6; ++i) {
                state.joints()[i].name("joint_" + std::to_string(i));
                state.joints()[i].position(std::sin(count * 0.1 + i * 0.5));
                state.joints()[i].velocity(std::cos(count * 0.1 + i * 0.5));
                state.joints()[i].effort(0.5 * std::sin(count * 0.2 + i));
            }

            writer.write(state);
            std::cout << "Published RobotState #" << count << " with " << state.joints().size() << " joints\r" << std::flush;

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
