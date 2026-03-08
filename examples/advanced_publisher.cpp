/**
 * @file advanced_publisher.cpp
 * @brief Publisher for AdvancedFeatures IDL demonstrating complex nested structures
 */

#include <chrono>
#include <cmath>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <random>
#include <thread>

#include <dds/dds.hpp>
#include "AdvancedFeatures.hpp"

namespace {
volatile std::sig_atomic_t g_running = 1;

void signal_handler(int) { g_running = 0; }

void print_usage(const char* prog_name) {
    std::cout << "Usage: " << prog_name << " [OPTIONS]\n"
              << "\n"
              << "Options:\n"
              << "  --topic <name>    Topic name (default: /telemetry/packet)\n"
              << "  --rate <hz>       Publishing rate in Hz (default: 5)\n"
              << "  --domain <id>     DDS domain ID (default: 0)\n"
              << "  --help            Show this help message\n"
              << std::endl;
}

struct Options {
    std::string topic_name = "/telemetry/packet";
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

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0.0, 1.0);

    try {
        dds::domain::DomainParticipant participant(opts.domain_id);
        dds::topic::Topic<demo::advanced::TelemetryPacket> topic(participant, opts.topic_name);
        dds::pub::Publisher publisher(participant);
        dds::pub::DataWriter<demo::advanced::TelemetryPacket> writer(publisher, topic);

        std::cout << "Publishing TelemetryPacket to: " << opts.topic_name << std::endl;

        demo::advanced::TelemetryPacket packet;
        auto sleep_duration = std::chrono::duration<double>(1.0 / opts.rate);
        int count = 0;

        while (g_running) {
            auto now = std::chrono::system_clock::now().time_since_epoch();
            auto sec = std::chrono::duration_cast<std::chrono::seconds>(now);
            auto nsec = std::chrono::duration_cast<std::chrono::nanoseconds>(now - sec);

            // Packet header
            packet.packet_id(static_cast<uint32_t>(count));
            packet.send_time().seconds(sec.count());
            packet.send_time().nanoseconds(static_cast<uint32_t>(nsec.count()));
            packet.receive_time(packet.send_time());

            // Source device (deeply nested)
            packet.source_device().device_name("telemetry_device_" + std::to_string(count % 4));
            packet.source_device().hardware_version("1.0.0");
            packet.source_device().firmware_version("2.1." + std::to_string(count % 10));
            packet.source_device().serial_number("SN" + std::to_string(10000 + count));
            packet.source_device().is_operational(true);
            packet.source_device().health_score(0.8f + dis(gen) * 0.19f);

            // Sub-devices
            packet.source_device().sub_devices().resize(3);
            for (int i = 0; i < 3; ++i) {
                packet.source_device().sub_devices()[i].device_id("sub_" + std::to_string(i));
                packet.source_device().sub_devices()[i].device_type(i == 0 ? "sensor" : (i == 1 ? "actuator" : "controller"));
                packet.source_device().sub_devices()[i].is_online(i != 2);

                packet.source_device().sub_devices()[i].capabilities().resize(2);
                packet.source_device().sub_devices()[i].capabilities()[0].name("temperature");
                demo::advanced::VariantValue temp_value;
                temp_value.double_val(20.0 + dis(gen) * 10.0);
                packet.source_device().sub_devices()[i].capabilities()[0].value(temp_value);
                packet.source_device().sub_devices()[i].capabilities()[0].unit("Celsius");
            }

            // Measurements (sequence of unions)
            packet.measurements().resize(5);
            for (int i = 0; i < 5; ++i) {
                demo::advanced::VariantValue measurement;
                if (i % 3 == 0) {
                    measurement.int_val(count * 100 + i);
                } else if (i % 3 == 1) {
                    measurement.double_val(dis(gen) * 100.0);
                } else {
                    measurement.string_val("measurement_" + std::to_string(i));
                }
                packet.measurements()[i] = measurement;
            }

            // Transform chain (sequence of nested transforms)
            packet.transform_chain().resize(3);
            for (int i = 0; i < 3; ++i) {
                // Translation
                packet.transform_chain()[i].translation().x(std::sin(count * 0.1 + i));
                packet.transform_chain()[i].translation().y(std::cos(count * 0.1 + i));
                packet.transform_chain()[i].translation().z(0.0);

                // Rotation
                packet.transform_chain()[i].rotation_euler().x(0.0);
                packet.transform_chain()[i].rotation_euler().y(0.0);
                packet.transform_chain()[i].rotation_euler().z(count * 0.05 + i * 0.1);

                // Scale
                packet.transform_chain()[i].scale().x(1.0);
                packet.transform_chain()[i].scale().y(1.0);
                packet.transform_chain()[i].scale().z(1.0);
            }

            // Raw payload
            packet.raw_payload().resize(32);
            for (int i = 0; i < 32; ++i) {
                packet.raw_payload()[i] = static_cast<uint8_t>((count + i) % 256);
            }

            // Packet metadata (key-value pairs)
            packet.packet_metadata().source_system("advanced_publisher");
            packet.packet_metadata().creation_time().seconds(sec.count());
            packet.packet_metadata().creation_time().nanoseconds(static_cast<uint32_t>(nsec.count()));
            packet.packet_metadata().entries().resize(3);
            packet.packet_metadata().entries()[0].key("sequence");
            demo::advanced::VariantValue seq_value;
            seq_value.int_val(count);
            packet.packet_metadata().entries()[0].value(seq_value);
            packet.packet_metadata().entries()[1].key("rate");
            demo::advanced::VariantValue rate_value;
            rate_value.double_val(opts.rate);
            packet.packet_metadata().entries()[1].value(rate_value);
            packet.packet_metadata().entries()[2].key("status");
            demo::advanced::VariantValue status_value;
            status_value.string_val("active");
            packet.packet_metadata().entries()[2].value(status_value);

            writer.write(packet);

            std::cout << "Published TelemetryPacket #" << count
                      << " [device: " << packet.source_device().device_name()
                      << ", measurements: " << packet.measurements().size()
                      << ", transforms: " << packet.transform_chain().size()
                      << "]\r" << std::flush;

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
