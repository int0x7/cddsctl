#include "InfoCommand.hpp"
#include <cddsctl/dds/Participant.hpp>
#include <cddsctl/dds/TopicDiscovery.hpp>
#include <cddsctl/dds/TypeSupport.hpp>
#include <cddsctl/core/Types.hpp>

#include <optionparser.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <mutex>
#include <vector>
#include <algorithm>
#include <cstring>

namespace cddsctl {
namespace cli {

namespace {

enum OptionIndex {
    UNKNOWN,
    HELP,
    DOMAIN,
    TIMEOUT,
    IDL
};

const option::Descriptor usage[] = {
    {UNKNOWN, 0, "", "", option::Arg::None,
        "Usage: cddsctl info <topic_name> [options]\n\n"
        "Show information about a DDS topic.\n\n"
        "Arguments:\n"
        "  topic_name            Name of the topic to query\n\n"
        "Options:"},
    {HELP, 0, "h", "help", option::Arg::None,
        "  -h, --help            show this help message and exit"},
    {DOMAIN, 0, "d", "domain", option::Arg::Optional,
        "  -d, --domain=ID       DDS domain ID (default: 0)"},
    {TIMEOUT, 0, "t", "timeout", option::Arg::Optional,
        "  -t, --timeout=SEC     discovery timeout in seconds (default: 2.0)"},
    {IDL, 0, "i", "idl", option::Arg::None,
        "  -i, --idl             show IDL type definition"},
    {0, 0, nullptr, nullptr, nullptr, nullptr}
};

// Endpoint info with QoS stored for later display
struct EndpointInfoLocal {
    core::Guid guid;
    dds_reliability_kind_t reliability_kind = DDS_RELIABILITY_BEST_EFFORT;
    dds_duration_t reliability_max_blocking_time = 0;
    dds_durability_kind_t durability_kind = DDS_DURABILITY_VOLATILE;
    dds_history_kind_t history_kind = DDS_HISTORY_KEEP_LAST;
    int32_t history_depth = 1;
    bool is_writer = false;
};

std::string format_reliability(const EndpointInfoLocal& ep) {
    switch (ep.reliability_kind) {
        case DDS_RELIABILITY_BEST_EFFORT:
            return "BEST_EFFORT";
        case DDS_RELIABILITY_RELIABLE:
            return "RELIABLE";
        default:
            return "UNKNOWN";
    }
}

std::string format_durability(const EndpointInfoLocal& ep) {
    switch (ep.durability_kind) {
        case DDS_DURABILITY_VOLATILE:
            return "VOLATILE";
        case DDS_DURABILITY_TRANSIENT_LOCAL:
            return "TRANSIENT_LOCAL";
        case DDS_DURABILITY_TRANSIENT:
            return "TRANSIENT";
        case DDS_DURABILITY_PERSISTENT:
            return "PERSISTENT";
        default:
            return "UNKNOWN";
    }
}

std::string format_history(const EndpointInfoLocal& ep) {
    switch (ep.history_kind) {
        case DDS_HISTORY_KEEP_LAST:
            return "KEEP_LAST (" + std::to_string(ep.history_depth) + ")";
        case DDS_HISTORY_KEEP_ALL:
            return "KEEP_ALL";
        default:
            return "UNKNOWN";
    }
}

}  // anonymous namespace

int InfoCommand::execute(int argc, char* argv[]) {
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
    double timeout_sec = 2.0;
    bool show_idl = options[IDL];

    if (options[DOMAIN] && options[DOMAIN].arg) {
        domain_id = std::stoi(options[DOMAIN].arg);
    }

    if (options[TIMEOUT] && options[TIMEOUT].arg) {
        timeout_sec = std::stod(options[TIMEOUT].arg);
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
                const char* flags[] = {"-h", "--help", "-i", "--idl", nullptr};
                for (const char** f = flags; *f; ++f) {
                    if (std::strcmp(a, *f) == 0) { is_flag = true; break; }
                }
                if (!is_flag) { ++i; }  // Skip the argument for -d/-t
            }
            continue;
        }
        topic_name = a;
        break;  // Take only the first non-option
    }

    // Check for topic name
    if (topic_name.empty()) {
        std::cerr << "Error: Missing topic name.\n\n";
        option::printUsage(std::cerr, usage);
        return 1;
    }

    // Create participant
    dds::Participant::Config part_config;
    part_config.domain_id = domain_id;
    part_config.participant_name = "cddsctl_info";
    part_config.enable_topic_discovery = true;

    dds::Participant participant(part_config);
    if (!participant.is_valid()) {
        std::cerr << "Error: Failed to create DDS participant on domain "
                  << domain_id << std::endl;
        return 1;
    }

    // Topic info collected during discovery
    std::mutex info_mutex;
    std::string discovered_type_name;
    std::vector<EndpointInfoLocal> endpoints;
    std::string generated_idl;  // Store IDL string, not type_info pointer

    // Create topic discovery
    dds::TopicDiscovery discovery(participant);

    discovery.set_endpoint_discovered_callback(
        [&info_mutex, &topic_name, &discovered_type_name, &endpoints, &generated_idl,
         &participant, show_idl](const dds::DiscoveredEndpoint& ep) {
            // Filter by topic name
            if (ep.topic_name != topic_name) {
                return;
            }

            std::lock_guard<std::mutex> lock(info_mutex);

            // Update type name and generate IDL immediately while type_info is valid
            if (discovered_type_name.empty()) {
                discovered_type_name = ep.type_name;

                // Generate IDL now, inside the callback, while type_info is still valid
                if (show_idl && ep.type_info) {
                    dds_topic_descriptor_t* descriptor =
                        dds::TypeSupport::create_topic_descriptor(
                            participant.handle(),
                            ep.type_name,
                            ep.type_info,
                            5000);

                    generated_idl = dds::TypeSupport::generate_idl_for_type(
                        ep.type_name, descriptor);

                    if (descriptor) {
                        dds::TypeSupport::free_topic_descriptor(descriptor);
                    }
                }
            }

            // Extract endpoint info
            EndpointInfoLocal local_ep;
            local_ep.guid = core::Guid::from_dds_guid(&ep.guid);
            local_ep.is_writer = ep.is_writer;

            // Extract QoS if available
            if (ep.qos) {
                dds_qget_reliability(ep.qos, &local_ep.reliability_kind,
                                     &local_ep.reliability_max_blocking_time);
                dds_qget_durability(ep.qos, &local_ep.durability_kind);
                dds_qget_history(ep.qos, &local_ep.history_kind, &local_ep.history_depth);
            }

            endpoints.push_back(std::move(local_ep));
        });

    // Start discovery
    discovery.start();

    // Wait for discovery timeout
    std::this_thread::sleep_for(
        std::chrono::milliseconds(static_cast<int64_t>(timeout_sec * 1000)));

    // Stop discovery
    discovery.stop();

    // Check if topic was found
    {
        std::lock_guard<std::mutex> lock(info_mutex);
        if (endpoints.empty()) {
            std::cout << "Topic '" << topic_name << "' not found on domain "
                      << domain_id << std::endl;
            return 0;
        }
    }

    // Separate publishers and subscribers
    std::vector<EndpointInfoLocal> publishers;
    std::vector<EndpointInfoLocal> subscribers;

    {
        std::lock_guard<std::mutex> lock(info_mutex);
        for (const auto& ep : endpoints) {
            if (ep.is_writer) {
                publishers.push_back(ep);
            } else {
                subscribers.push_back(ep);
            }
        }
    }

    // Sort by GUID for consistent output
    auto guid_comparator = [](const EndpointInfoLocal& a, const EndpointInfoLocal& b) {
        return a.guid < b.guid;
    };
    std::sort(publishers.begin(), publishers.end(), guid_comparator);
    std::sort(subscribers.begin(), subscribers.end(), guid_comparator);

    // Output results
    std::cout << "Topic: " << topic_name << "\n";
    std::cout << "Type:  " << discovered_type_name << "\n";

    // Display IDL if requested (already generated during discovery)
    if (show_idl) {
        if (generated_idl.empty()) {
            // Fallback: generate placeholder if IDL wasn't generated during discovery
            generated_idl = dds::TypeSupport::generate_idl_for_type(
                discovered_type_name, nullptr);
        }
        std::cout << "\nIDL:\n" << generated_idl;
    }

    std::cout << "\nPublishers:\n";
    if (publishers.empty()) {
        std::cout << "  (none)\n";
    } else {
        for (const auto& ep : publishers) {
            std::cout << "  * " << ep.guid.to_string() << "\n";
            std::cout << "      Reliability: " << format_reliability(ep) << "\n";
            std::cout << "      Durability:  " << format_durability(ep) << "\n";
            std::cout << "      History:     " << format_history(ep) << "\n";
        }
    }

    std::cout << "\nSubscribers:\n";
    if (subscribers.empty()) {
        std::cout << "  (none)\n";
    } else {
        for (const auto& ep : subscribers) {
            std::cout << "  * " << ep.guid.to_string() << "\n";
            std::cout << "      Reliability: " << format_reliability(ep) << "\n";
            std::cout << "      Durability:  " << format_durability(ep) << "\n";
            std::cout << "      History:     " << format_history(ep) << "\n";
        }
    }

    return 0;
}

}  // namespace cli
}  // namespace cddsctl
