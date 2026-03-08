#include "EchoCommand.hpp"
#include <cddsctl/dds/Participant.hpp>
#include <cddsctl/dds/TopicDiscovery.hpp>
#include <cddsctl/dds/RawDataReader.hpp>
#include <cddsctl/dds/TypeSupport.hpp>
#include <cddsctl/dds/YamlPrinter.hpp>
#include <cddsctl/dds/JsonPrinter.hpp>
#include <cddsctl/core/Log.hpp>

#include <dds/ddsi/ddsi_serdata.h>
#include <dds/ddsi/ddsi_serdata_default.h>
#include <dds/ddsi/ddsi_cdrstream.h>
#include <dds/ddsrt/iovec.h>

#ifdef DDS_HAS_SHM
#include <dds/ddsi/ddsi_shm_transport.h>
#endif

#include <optionparser.h>
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <mutex>
#include <atomic>
#include <csignal>
#include <cstring>
#include <vector>

namespace cddsctl {
namespace cli {

// Signal handler for graceful shutdown
static std::atomic<bool> g_echo_running{true};

static void echo_signal_handler(int) {
    g_echo_running.store(false);
}

namespace {

enum OptionIndex {
    UNKNOWN,
    HELP,
    DOMAIN,
    COUNT,
    TIMEOUT,
    NO_TIMESTAMP,
    FORMAT
};

enum class OutputFormat {
    YAML,
    JSON
};

const option::Descriptor usage[] = {
    {UNKNOWN, 0, "", "", option::Arg::None,
        "Usage: cddsctl echo <topic_name> [options]\n\n"
        "Print messages from a DDS topic.\n\n"
        "Options:"},
    {HELP, 0, "h", "help", option::Arg::None,
        "  -h, --help            show this help message and exit"},
    {DOMAIN, 0, "d", "domain", option::Arg::Optional,
        "  -d, --domain=ID       DDS domain ID (default: 0)"},
    {COUNT, 0, "n", "count", option::Arg::Optional,
        "  -n, --count=N         print N messages then exit (default: unlimited)"},
    {TIMEOUT, 0, "t", "timeout", option::Arg::Optional,
        "  -t, --timeout=SEC     discovery timeout in seconds (default: 2.0)"},
    {NO_TIMESTAMP, 0, "", "no-timestamp", option::Arg::None,
        "  --no-timestamp        don't show timestamps"},
    {FORMAT, 0, "F", "format", option::Arg::Optional,
        "  -F, --format=FMT      output format: yaml, json (default: yaml)"},
    {0, 0, nullptr, nullptr, nullptr, nullptr}
};

/**
 * @brief Take CDR samples from reader and print them
 * @param reader DDS reader entity handle
 * @param show_timestamp Whether to show timestamp for each message
 * @param descriptor Topic descriptor for formatted output (optional)
 * @param format Output format (YAML, JSON, or RAW)
 * @param type_name Type name for identifying the main struct (optional)
 * @return Number of samples printed
 */
size_t take_and_print(dds_entity_t reader, bool show_timestamp,
                      const dds_topic_descriptor_t* descriptor,
                      OutputFormat format,
                      const char* type_name = nullptr) {
    constexpr size_t MAX_SAMPLES = 32;
    dds_sample_info_t infos[MAX_SAMPLES];
    struct ddsi_serdata* serdatas[MAX_SAMPLES];

    dds_return_t n = dds_takecdr(reader, serdatas, MAX_SAMPLES, infos, DDS_ANY_STATE);
    if (n <= 0) {
        return 0;
    }

    size_t printed = 0;
    for (int32_t i = 0; i < n; ++i) {
        if (!infos[i].valid_data) {
            ddsi_serdata_unref(serdatas[i]);
            continue;
        }

        std::cout << "---\n";

        if (show_timestamp) {
            int64_t ts = infos[i].source_timestamp;
            int64_t sec = ts / 1000000000LL;
            int64_t nsec = ts % 1000000000LL;
            if (nsec < 0) {
                sec -= 1;
                nsec += 1000000000LL;
            }
            std::cout << "[" << sec << "." << std::setfill('0')
                      << std::setw(9) << nsec << "]\n";
        }

        // Get sertype_default from the sertype (first member 'c' is ddsi_sertype)
        const ddsi_sertype_default* sertype_default =
            reinterpret_cast<const ddsi_sertype_default*>(serdatas[i]->type);

        // Try to get CDR data - it might be in serdata or in SHM
        const uint8_t* cdr_data = nullptr;
        size_t cdr_len = 0;
        uint32_t xcdr_version = CDR_ENC_VERSION_1;
        bool need_unref = false;
        ddsrt_iovec_t ref = {};

        // First try ddsi_serdata_to_ser_ref
        uint32_t ser_size = ddsi_serdata_size(serdatas[i]);
        if (ser_size > 4) {
            ddsi_serdata_to_ser_ref(serdatas[i], 0, ser_size, &ref);
            if (ref.iov_base && ref.iov_len > 4) {
                cdr_data = static_cast<const uint8_t*>(ref.iov_base);
                cdr_len = ref.iov_len;
                need_unref = true;
            }
        }

#ifdef DDS_HAS_SHM
        // If data is in SHM, we need to get it from iox_chunk
        if (cdr_len <= 4 && serdatas[i]->iox_chunk != nullptr) {
            iceoryx_header_t* iox_hdr = iceoryx_header_from_chunk(serdatas[i]->iox_chunk);
            if (iox_hdr->shm_data_state == IOX_CHUNK_CONTAINS_SERIALIZED_DATA) {
                // Data is serialized CDR in SHM
                cdr_data = static_cast<const uint8_t*>(serdatas[i]->iox_chunk);
                cdr_len = iox_hdr->data_size;
            } else if (iox_hdr->shm_data_state == IOX_CHUNK_CONTAINS_RAW_DATA) {
                // Data is raw (not serialized), need to serialize it
                const struct ddsi_sertype* type = serdatas[i]->type;
                if (type && type->ops->get_serialized_size && type->ops->serialize_into) {
                    size_t sz = ddsi_sertype_get_serialized_size(type, serdatas[i]->iox_chunk);
                    if (sz > 4 && sz != SIZE_MAX) {
                        // Allocate temp buffer and serialize
                        static thread_local std::vector<uint8_t> temp_buf;
                        temp_buf.resize(sz);
                        if (ddsi_sertype_serialize_into(type, serdatas[i]->iox_chunk,
                                                        temp_buf.data(), sz)) {
                            cdr_data = temp_buf.data();
                            cdr_len = sz;
                        }
                    }
                }
            }
        }
#endif

        // Now print if we have data
        if (cdr_data && cdr_len > 4) {
            // First 4 bytes are CDR encapsulation header
            uint8_t encap = cdr_data[1];
            xcdr_version = (encap >= 0x06) ? CDR_ENC_VERSION_2 : CDR_ENC_VERSION_1;

            if (format == OutputFormat::JSON) {
                bool printed_json = false;
                if (dds::JsonPrinter::is_available(descriptor)) {
                    std::string json_str = dds::JsonPrinter::format(
                        cdr_data + 4, cdr_len - 4, descriptor, xcdr_version, type_name);
                    if (!json_str.empty()) {
                        std::cout << json_str;
                        printed_json = true;
                    }
                }
                if (!printed_json) {
                    std::cout << "{\"error\": \"type metadata not available\"}\n";
                }
            } else {
                // YAML (default)
                bool printed_yaml = false;
                if (dds::YamlPrinter::is_available(descriptor)) {
                    std::string yaml = dds::YamlPrinter::format(
                        cdr_data + 4, cdr_len - 4, descriptor, xcdr_version, type_name);
                    if (!yaml.empty()) {
                        std::cout << yaml;
                        printed_yaml = true;
                    }
                }

                // Fallback to compact format
                if (!printed_yaml) {
                    dds_istream_t is;
                    dds_istream_init(&is, static_cast<uint32_t>(cdr_len - 4),
                                     cdr_data + 4, xcdr_version);

                    char buf[16384];
                    buf[0] = '\0';
                    size_t written = dds_stream_print_sample(&is, sertype_default, buf, sizeof(buf));

                    if (written > 0 && written < sizeof(buf)) {
                        std::cout << buf << "\n";
                    } else if (written >= sizeof(buf)) {
                        buf[sizeof(buf) - 1] = '\0';
                        std::cout << buf << "...(truncated)\n";
                    }

                    dds_istream_fini(&is);
                }
            }
        }

        if (need_unref) {
            ddsi_serdata_to_ser_unref(serdatas[i], &ref);
        }

        ddsi_serdata_unref(serdatas[i]);
        ++printed;
    }

    return printed;
}

}  // anonymous namespace

int EchoCommand::execute(int argc, char* argv[]) {
    // Reset global state
    g_echo_running.store(true);

    option::Stats opt_stats(usage, argc, argv);
    std::vector<option::Option> options(opt_stats.options_max);
    std::vector<option::Option> buffer(opt_stats.buffer_max);
    option::Parser parse(usage, argc, argv, options.data(), buffer.data());

    if (parse.error()) {
        return 1;
    }

    if (options[HELP]) {
        option::printUsage(std::cout, usage);
        return 0;
    }

    // Parse options
    int32_t domain_id = 0;
    int64_t max_count = 0;  // 0 = unlimited
    double timeout_sec = 2.0;
    bool show_timestamp = !options[NO_TIMESTAMP];

    if (options[DOMAIN] && options[DOMAIN].arg) {
        domain_id = std::stoi(options[DOMAIN].arg);
    }

    if (options[COUNT] && options[COUNT].arg) {
        max_count = std::stoll(options[COUNT].arg);
        if (max_count < 0) {
            max_count = 0;
        }
    }

    if (options[TIMEOUT] && options[TIMEOUT].arg) {
        timeout_sec = std::stod(options[TIMEOUT].arg);
    }

    OutputFormat output_format = OutputFormat::YAML;
    if (options[FORMAT] && options[FORMAT].arg) {
        std::string fmt = options[FORMAT].arg;
        if (fmt == "json") {
            output_format = OutputFormat::JSON;
        } else if (fmt != "yaml") {
            std::cerr << "Error: unknown format '" << fmt << "' (use yaml or json)\n";
            return 1;
        }
    }

    // Collect non-option arguments (topic name)
    std::string topic_name;
    for (int i = 0; i < argc; ++i) {
        const char* a = argv[i];
        // Skip options and their arguments
        if (a[0] == '-') {
            // If this option has no '=', it may have a separate argument
            if (std::strchr(a, '=') == nullptr) {
                bool is_flag = false;
                const char* flags[] = {"-h", "--help", "--no-timestamp", nullptr};
                for (const char** f = flags; *f; ++f) {
                    if (std::strcmp(a, *f) == 0) { is_flag = true; break; }
                }
                if (!is_flag) { ++i; }  // Skip the argument for -d/-n/-t
            }
            continue;
        }
        topic_name = a;
        break;  // Take only the first non-option
    }

    // Check for topic name
    if (topic_name.empty()) {
        std::cerr << "Error: topic name is required\n\n";
        option::printUsage(std::cerr, usage);
        return 1;
    }

    // Create participant
    dds::Participant::Config part_config;
    part_config.domain_id = domain_id;
    part_config.participant_name = "cddsctl_echo";
    part_config.enable_topic_discovery = true;

    dds::Participant participant(part_config);
    if (!participant.is_valid()) {
        std::cerr << "Error: Failed to create DDS participant on domain "
                  << domain_id << std::endl;
        return 1;
    }

    // Track reader creation
    std::mutex reader_mutex;
    std::unique_ptr<dds::RawDataReader> reader;
    dds_topic_descriptor_t* topic_descriptor = nullptr;
    std::string discovered_type_name;
    bool topic_found = false;

    // Create topic discovery
    dds::TopicDiscovery discovery(participant);

    discovery.set_endpoint_discovered_callback(
        [&](const dds::DiscoveredEndpoint& ep) {
            if (ep.topic_name != topic_name) {
                return;
            }

            std::lock_guard<std::mutex> lock(reader_mutex);
            if (reader) {
                return;  // Already created
            }

            // Create topic descriptor for YAML formatting
            topic_descriptor = dds::TypeSupport::create_topic_descriptor(
                participant.handle(), ep.type_name, ep.type_info);

            // Create reader in callback - type_info is only valid here
            dds::RawDataReader::Config cfg;
            cfg.topic_name = ep.topic_name;
            cfg.type_name = ep.type_name;
            cfg.type_info = ep.type_info;
            cfg.reliability = core::ReliabilityKind::BestEffort;
            cfg.durability = core::DurabilityKind::Volatile;
            cfg.history_depth = 100;

            reader = std::make_unique<dds::RawDataReader>(participant, cfg);
            if (reader && reader->is_valid()) {
                topic_found = true;
                discovered_type_name = ep.type_name;  // Store type name for YAML formatting
            } else {
                reader.reset();
            }
        });

    // Setup signal handler for Ctrl+C
    std::signal(SIGINT, echo_signal_handler);
    std::signal(SIGTERM, echo_signal_handler);

    // Start discovery
    discovery.start();

    // Wait for topic discovery or timeout
    auto start = std::chrono::steady_clock::now();
    auto timeout = std::chrono::milliseconds(static_cast<int64_t>(timeout_sec * 1000));

    while (g_echo_running.load() && !topic_found) {
        auto elapsed = std::chrono::steady_clock::now() - start;
        if (elapsed >= timeout) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    if (!topic_found) {
        std::cout << "Topic '" << topic_name << "' not found on domain " << domain_id << std::endl;
        discovery.stop();
        return 1;
    }

    // Main loop: read and print messages
    int64_t printed_count = 0;

    while (g_echo_running.load()) {
        // Check if we've reached the message count limit
        if (max_count > 0 && printed_count >= max_count) {
            break;
        }

        {
            std::lock_guard<std::mutex> lock(reader_mutex);
            if (reader && reader->is_valid()) {
                size_t samples = take_and_print(reader->handle(), show_timestamp,
                    topic_descriptor, output_format, discovered_type_name.c_str());
                printed_count += samples;
            }
        }

        // Small sleep to avoid busy-waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Cleanup
    discovery.stop();

    if (topic_descriptor) {
        dds::TypeSupport::free_topic_descriptor(topic_descriptor);
    }

    return 0;
}

}  // namespace cli
}  // namespace cddsctl
