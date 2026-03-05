#include "ListCommand.hpp"
#include <cddsctl/dds/Participant.hpp>
#include <cddsctl/dds/TopicDiscovery.hpp>
#include <cddsctl/core/Log.hpp>

#include <optionparser.h>
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <mutex>
#include <map>
#include <vector>
#include <atomic>
#include <csignal>

namespace cddsctl {
namespace cli {

// Signal handler for graceful shutdown
static std::atomic<bool> g_list_running{true};

static void list_signal_handler(int) {
    g_list_running.store(false);
}

namespace {

enum OptionIndex {
    UNKNOWN,
    HELP,
    DOMAIN,
    TIMEOUT,
    VERBOSE
};

const option::Descriptor usage[] = {
    {UNKNOWN, 0, "", "", option::Arg::None,
        "Usage: cddsctl list [options]\n\n"
        "List available DDS topics.\n\n"
        "Options:"},
    {HELP, 0, "h", "help", option::Arg::None,
        "  -h, --help            show this help message and exit"},
    {DOMAIN, 0, "d", "domain", option::Arg::Optional,
        "  -d, --domain=ID       DDS domain ID (default: 0)"},
    {TIMEOUT, 0, "t", "timeout", option::Arg::Optional,
        "  -t, --timeout=SEC     discovery timeout in seconds (default: 2.0)"},
    {VERBOSE, 0, "v", "verbose", option::Arg::None,
        "  -v, --verbose         show detailed information (type name)"},
    {0, 0, nullptr, nullptr, nullptr, nullptr}
};

// Topic information collected during discovery
struct TopicInfo {
    std::string type_name;
};

}  // anonymous namespace

int ListCommand::execute(int argc, char* argv[]) {
    // Reset global state
    g_list_running.store(true);

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
    bool verbose = options[VERBOSE];

    if (options[DOMAIN] && options[DOMAIN].arg) {
        domain_id = std::stoi(options[DOMAIN].arg);
    }

    if (options[TIMEOUT] && options[TIMEOUT].arg) {
        timeout_sec = std::stod(options[TIMEOUT].arg);
    }

    // Create participant
    dds::Participant::Config part_config;
    part_config.domain_id = domain_id;
    part_config.participant_name = "cddsctl_list";
    part_config.enable_topic_discovery = true;

    dds::Participant participant(part_config);
    if (!participant.is_valid()) {
        std::cerr << "Error: Failed to create DDS participant on domain "
                  << domain_id << std::endl;
        return 1;
    }

    // Track discovered topics
    std::mutex topics_mutex;
    std::map<std::string, TopicInfo> topics;

    // Create topic discovery
    dds::TopicDiscovery discovery(participant);

    discovery.set_endpoint_discovered_callback(
        [&topics_mutex, &topics](const dds::DiscoveredEndpoint& ep) {
            std::lock_guard<std::mutex> lock(topics_mutex);
            auto& info = topics[ep.topic_name];
            if (info.type_name.empty()) {
                info.type_name = ep.type_name;
            }
        });

    // Setup signal handler for early exit
    std::signal(SIGINT, list_signal_handler);
    std::signal(SIGTERM, list_signal_handler);

    // Start discovery
    discovery.start();

    // Wait for timeout or interrupt
    auto start = std::chrono::steady_clock::now();
    auto timeout = std::chrono::milliseconds(static_cast<int64_t>(timeout_sec * 1000));

    while (g_list_running.load()) {
        auto elapsed = std::chrono::steady_clock::now() - start;
        if (elapsed >= timeout) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // Stop discovery
    discovery.stop();

    // Collect and sort topic names
    std::vector<std::string> topic_names;
    {
        std::lock_guard<std::mutex> lock(topics_mutex);
        topic_names.reserve(topics.size());
        for (const auto& [name, info] : topics) {
            topic_names.push_back(name);
        }
    }
    std::sort(topic_names.begin(), topic_names.end());

    // Output results
    if (topic_names.empty()) {
        std::cout << "No topics discovered on domain " << domain_id << std::endl;
        return 0;
    }

    std::lock_guard<std::mutex> lock(topics_mutex);
    for (const auto& name : topic_names) {
        if (verbose) {
            std::cout << name << " [" << topics[name].type_name << "]" << std::endl;
        } else {
            std::cout << name << std::endl;
        }
    }

    return 0;
}

}  // namespace cli
}  // namespace cddsctl
