/**
 * @file array_publisher.cpp
 * @brief Publisher for ArraysAndSequences IDL demonstrating array types
 */

#include <chrono>
#include <cmath>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>

#include <dds/dds.hpp>
#include "ArraysAndSequences.hpp"

namespace {
volatile std::sig_atomic_t g_running = 1;

void signal_handler(int) { g_running = 0; }

void print_usage(const char* prog_name) {
    std::cout << "Usage: " << prog_name << " [OPTIONS]\n"
              << "\n"
              << "Options:\n"
              << "  --topic <name>    Topic name (default: /arrays/data)\n"
              << "  --rate <hz>       Publishing rate in Hz (default: 10)\n"
              << "  --domain <id>     DDS domain ID (default: 0)\n"
              << "  --help            Show this help message\n"
              << std::endl;
}

struct Options {
    std::string topic_name = "/arrays/data";
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
        dds::topic::Topic<demo::arrays::MixedArrays> topic(participant, opts.topic_name);
        dds::pub::Publisher publisher(participant);
        dds::pub::DataWriter<demo::arrays::MixedArrays> writer(publisher, topic);

        std::cout << "Publishing MixedArrays to: " << opts.topic_name << std::endl;

        demo::arrays::MixedArrays data;
        auto sleep_duration = std::chrono::duration<double>(1.0 / opts.rate);
        int count = 0;

        while (g_running) {
            // Fill fixed-size current pose [x, y, z, roll, pitch, yaw]
            data.current_pose()[0] = std::sin(count * 0.1);
            data.current_pose()[1] = std::cos(count * 0.1);
            data.current_pose()[2] = 0.5 + 0.1 * std::sin(count * 0.05);
            data.current_pose()[3] = 0.0;
            data.current_pose()[4] = 0.0;
            data.current_pose()[5] = count * 0.1;

            // Fill pose history (sequence of Pose6D structs)
            data.pose_history().resize(5);
            for (size_t i = 0; i < 5; ++i) {
                data.pose_history()[i].values()[0] = std::sin((count - i) * 0.1);
                data.pose_history()[i].values()[1] = std::cos((count - i) * 0.1);
                data.pose_history()[i].values()[2] = 0.0;
                data.pose_history()[i].values()[3] = 0.0;
                data.pose_history()[i].values()[4] = 0.0;
                data.pose_history()[i].values()[5] = (count - i) * 0.1;
            }

            // Fill 2D grid data (sequence of sequences)
            data.grid_data().resize(3);
            for (size_t row = 0; row < 3; ++row) {
                data.grid_data()[row].resize(4);
                for (size_t col = 0; col < 4; ++col) {
                    data.grid_data()[row][col] = std::sin(count * 0.1 + row * 0.2 + col * 0.1);
                }
            }

            // Fixed sensor IDs
            data.sensor_ids()[0] = 101;
            data.sensor_ids()[1] = 102;
            data.sensor_ids()[2] = 103;
            data.sensor_ids()[3] = 104;

            // Variable timestamps
            data.timestamps().resize(4);
            auto now = std::chrono::system_clock::now().time_since_epoch();
            double sec = std::chrono::duration<double>(now).count();
            for (size_t i = 0; i < 4; ++i) {
                data.timestamps()[i] = sec - i * 0.01;
            }

            writer.write(data);
            std::cout << "Published MixedArrays #" << count
                      << " (history: " << data.pose_history().size()
                      << ", grid: " << data.grid_data().size() << "x"
                      << (data.grid_data().empty() ? 0 : data.grid_data()[0].size()) << ")\r"
                      << std::flush;

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
