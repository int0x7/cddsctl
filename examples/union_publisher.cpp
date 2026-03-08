/**
 * @file union_publisher.cpp
 * @brief Publisher for UnionType IDL demonstrating discriminated unions
 */

#include <chrono>
#include <csignal>
#include <cstdlib>
#include <iostream>
#include <thread>

#include <dds/dds.hpp>
#include "UnionType.hpp"

namespace {
volatile std::sig_atomic_t g_running = 1;

void signal_handler(int) { g_running = 0; }

void print_usage(const char* prog_name) {
    std::cout << "Usage: " << prog_name << " [OPTIONS]\n"
              << "\n"
              << "Options:\n"
              << "  --topic <name>    Topic name (default: /sensors/reading)\n"
              << "  --rate <hz>       Publishing rate in Hz (default: 5)\n"
              << "  --domain <id>     DDS domain ID (default: 0)\n"
              << "  --help            Show this help message\n"
              << std::endl;
}

struct Options {
    std::string topic_name = "/sensors/reading";
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

const char* type_to_string(demo::unions::DataType t) {
    switch (t) {
        case demo::unions::DataType::TYPE_EMPTY: return "EMPTY";
        case demo::unions::DataType::TYPE_INTEGER: return "INTEGER";
        case demo::unions::DataType::TYPE_FLOAT: return "FLOAT";
        case demo::unions::DataType::TYPE_STRING: return "STRING";
        case demo::unions::DataType::TYPE_BINARY: return "BINARY";
    }
    return "UNKNOWN";
}

const char* result_to_string(demo::unions::ResultStatus r) {
    switch (r) {
        case demo::unions::ResultStatus::RESULT_SUCCESS: return "SUCCESS";
        case demo::unions::ResultStatus::RESULT_ERROR_CODE: return "ERROR_CODE";
        case demo::unions::ResultStatus::RESULT_ERROR_MESSAGE: return "ERROR_MESSAGE";
    }
    return "UNKNOWN";
}
}  // namespace

int main(int argc, char* argv[]) {
    Options opts;
    if (!parse_args(argc, argv, opts)) return 1;

    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    try {
        dds::domain::DomainParticipant participant(opts.domain_id);
        dds::topic::Topic<demo::unions::CommandResponse> topic(participant, opts.topic_name);
        dds::pub::Publisher publisher(participant);
        dds::pub::DataWriter<demo::unions::CommandResponse> writer(publisher, topic);

        std::cout << "Publishing CommandResponse to: " << opts.topic_name << std::endl;

        demo::unions::CommandResponse response;
        auto sleep_duration = std::chrono::duration<double>(1.0 / opts.rate);
        int count = 0;

        // Cycle through different union types
        const demo::unions::DataType data_types[] = {
            demo::unions::DataType::TYPE_INTEGER,
            demo::unions::DataType::TYPE_FLOAT,
            demo::unions::DataType::TYPE_STRING,
            demo::unions::DataType::TYPE_BINARY
        };

        while (g_running) {
            try {
            auto now = std::chrono::system_clock::now().time_since_epoch();
            long long sec = std::chrono::duration_cast<std::chrono::seconds>(now).count();

            response.sequence_num(count);
            response.command_name("command_" + std::to_string(count % 10));

            // Alternate between success and error
            // Note: Must create new union objects - cannot change discriminator on existing object
            demo::unions::CommandResult result;
            if (count % 3 == 0) {
                // Error response with error code
                result.error_code(100 + count);
            } else if (count % 3 == 1) {
                // Error response with message
                result.error_message("Error in cycle " + std::to_string(count));
            } else {
                // Success response
                result.success(true);
            }
            response.result(result);

            // Set return value based on type cycle
            demo::unions::DataType current_type = data_types[count % 4];
            demo::unions::DataValue return_value;

            switch (current_type) {
                case demo::unions::DataType::TYPE_INTEGER:
                    return_value.int_val(count * 100);
                    break;
                case demo::unions::DataType::TYPE_FLOAT:
                    return_value.float_val(count * 0.123);
                    break;
                case demo::unions::DataType::TYPE_STRING:
                    return_value.str_val("Result: " + std::to_string(count));
                    break;
                case demo::unions::DataType::TYPE_BINARY: {
                    std::vector<uint8_t> binary_data(8);
                    for (int i = 0; i < 8; ++i) {
                        binary_data[i] = static_cast<uint8_t>((count + i) % 256);
                    }
                    return_value.binary_val(binary_data);
                    break;
                }
                default:
                    return_value.empty_flag(true);
                    break;
            }
            response.return_value(return_value);

            writer.write(response);

            std::cout << "Published CommandResponse #" << count
                      << " [result: " << result_to_string(response.result()._d())
                      << ", return: " << type_to_string(response.return_value()._d())
                      << "]\r" << std::flush;

            ++count;
            std::this_thread::sleep_for(sleep_duration);
            } catch (const std::exception& e) {
                std::cerr << "\nException at count=" << count << ": " << e.what() << std::endl;
                throw;
            }
        }

        std::cout << "\nPublisher stopped. Published " << count << " messages." << std::endl;
        return 0;

    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
}
