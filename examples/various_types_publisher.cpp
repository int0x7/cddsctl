/**
 * @file various_types_publisher.cpp
 * @brief Publisher for VariousTypes IDL demonstrating primitive types
 */

#include <chrono>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <thread>

#include <dds/dds.hpp>
#include "VariousTypes.hpp"

namespace {
volatile std::sig_atomic_t g_running = 1;

void signal_handler(int) { g_running = 0; }

void print_usage(const char* prog_name) {
    std::cout << "Usage: " << prog_name << " [OPTIONS]\n"
              << "\n"
              << "Options:\n"
              << "  --topic <name>    Topic name (default: /types/diagnostic)\n"
              << "  --rate <hz>       Publishing rate in Hz (default: 5)\n"
              << "  --domain <id>     DDS domain ID (default: 0)\n"
              << "  --help            Show this help message\n"
              << std::endl;
}

struct Options {
    std::string topic_name = "/types/diagnostic";
    double rate = 5.0;
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
        dds::topic::Topic<demo::types::DiagnosticData> topic(participant, opts.topic_name);
        dds::pub::Publisher publisher(participant);
        dds::pub::DataWriter<demo::types::DiagnosticData> writer(publisher, topic);

        std::cout << "Publishing DiagnosticData to: " << opts.topic_name << std::endl;

        demo::types::DiagnosticData data;
        auto sleep_duration = std::chrono::duration<double>(1.0 / opts.rate);
        int count = 0;

        while (g_running) {
            auto now = std::chrono::system_clock::now().time_since_epoch();
            long long sec = std::chrono::duration_cast<std::chrono::seconds>(now).count();

            data.timestamp(sec * 1000000000LL + count * 1000000);  // nanoseconds
            data.source("various_types_publisher");
            data.is_valid(count % 2 == 0);

            // Integer types
            data.integers().short_val(static_cast<int16_t>(count % 32767));
            data.integers().long_val(count);
            data.integers().longlong_val(sec * 1000000LL + count);
            data.integers().ushort_val(static_cast<uint16_t>(count % 65535));
            data.integers().ulong_val(static_cast<uint32_t>(count));
            data.integers().ulonglong_val(static_cast<uint64_t>(sec));
            data.integers().char_val('A' + (count % 26));
            data.integers().bool_val(count % 3 == 0);
            data.integers().byte_val(static_cast<uint8_t>(count % 256));

            // Floating types
            data.floats().float_val(static_cast<float>(count) * 0.5f);
            data.floats().double_val(static_cast<double>(count) * 0.001);

            // String types
            data.strings().unbounded_str("Message #" + std::to_string(count));

            // Raw status bytes
            for (int i = 0; i < 8; ++i) {
                data.raw_status()[i] = static_cast<uint8_t>((count + i) % 256);
            }

            writer.write(data);
            std::cout << "Published DiagnosticData #" << count << " (timestamp: " << data.timestamp() << ")\r" << std::flush;

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