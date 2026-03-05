/**
 * @file test_publisher.cpp
 * @brief Test data publisher for cddsctl record end-to-end testing
 *
 * This program publishes SensorData messages to DDS for testing
 * the recording functionality of cddsctl.
 */

#include <chrono>
#include <cmath>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>

#include <dds/dds.hpp>
#include "TestTypes.hpp"

namespace {
volatile std::sig_atomic_t g_running = 1;

void signal_handler(int /*signal*/) {
    g_running = 0;
}

void print_usage(const char* prog_name) {
    std::cout << "Usage: " << prog_name << " [OPTIONS]\n"
              << "\n"
              << "Options:\n"
              << "  --topic <name>    Topic name (default: /test/sensor)\n"
              << "  --count <n>       Number of messages to publish (default: 100, 0=infinite)\n"
              << "  --rate <hz>       Publishing rate in Hz (default: 10)\n"
              << "  --domain <id>     DDS domain ID (default: 0)\n"
              << "  --help            Show this help message\n"
              << std::endl;
}

struct Options {
    std::string topic_name = "/test/sensor";
    int count = 100;
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
        } else if (arg == "--count" && i + 1 < argc) {
            opts.count = std::atoi(argv[++i]);
        } else if (arg == "--rate" && i + 1 < argc) {
            opts.rate = std::atof(argv[++i]);
        } else if (arg == "--domain" && i + 1 < argc) {
            opts.domain_id = std::atoi(argv[++i]);
        } else {
            std::cerr << "Unknown option: " << arg << std::endl;
            print_usage(argv[0]);
            return false;
        }
    }
    return true;
}

}  // namespace

int main(int argc, char* argv[]) {
    Options opts;
    if (!parse_args(argc, argv, opts)) {
        return 1;
    }

    // Set up signal handler for graceful shutdown
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    try {
        // Create DDS participant
        dds::domain::DomainParticipant participant(opts.domain_id);

        // Create topic
        dds::topic::Topic<test::SensorData> topic(participant, opts.topic_name);

        // Create publisher and data writer
        dds::pub::Publisher publisher(participant);
        dds::pub::DataWriter<test::SensorData> writer(publisher, topic);

        std::cout << "Publishing to topic: " << opts.topic_name << std::endl;
        std::cout << "Domain: " << opts.domain_id << std::endl;
        std::cout << "Rate: " << opts.rate << " Hz" << std::endl;
        std::cout << "Count: " << (opts.count == 0 ? "infinite" : std::to_string(opts.count)) << std::endl;
        std::cout << "Press Ctrl+C to stop..." << std::endl;

        // Calculate sleep duration
        auto sleep_duration = std::chrono::duration<double>(1.0 / opts.rate);

        // Publish messages
        int msg_id = 0;
        auto start_time = std::chrono::steady_clock::now();

        while (g_running && (opts.count == 0 || msg_id < opts.count)) {
            // Create message
            test::SensorData msg;
            msg.id(msg_id);

            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration<double>(now - start_time).count();
            msg.timestamp(elapsed);

            // Generate some test values
            std::array<float, 3> values = {
                static_cast<float>(std::sin(elapsed)),
                static_cast<float>(std::cos(elapsed)),
                static_cast<float>(msg_id % 100) / 100.0f
            };
            msg.values(values);
            msg.name("sensor_" + std::to_string(msg_id % 10));

            // Write message
            writer.write(msg);

            std::cout << "\rPublished message " << (msg_id + 1)
                      << " (id=" << msg.id()
                      << ", timestamp=" << msg.timestamp()
                      << ")" << std::flush;

            ++msg_id;

            // Sleep until next publish time
            std::this_thread::sleep_for(sleep_duration);
        }

        std::cout << std::endl;
        std::cout << "Published " << msg_id << " messages total." << std::endl;

    } catch (const dds::core::Exception& e) {
        std::cerr << "DDS Exception: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
