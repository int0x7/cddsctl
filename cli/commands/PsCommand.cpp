#include "PsCommand.hpp"
#include <cddsctl/dds/Participant.hpp>
#include <cddsctl/core/Log.hpp>
#include <cddsctl/core/Types.hpp>

#include <dds/dds.h>
#include <dds/ddsc/dds_public_qos.h>
#include <optionparser.h>
#include <iostream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <mutex>
#include <map>
#include <set>
#include <vector>
#include <atomic>
#include <csignal>
#include <cstring>

namespace cddsctl {
namespace cli {

static std::atomic<bool> g_ps_running{true};

static void ps_signal_handler(int) {
    g_ps_running.store(false);
}

namespace {

enum OptionIndex {
    UNKNOWN,
    HELP,
    DOMAIN,
    TIMEOUT,
    TOPIC_FILTER,
    SHOW_SELF,
    VERBOSE
};

const option::Descriptor usage[] = {
    {UNKNOWN, 0, "", "", option::Arg::None,
        "Usage: cddsctl ps [options]\n\n"
        "Show DDS participants and applications in the network.\n\n"
        "Options:"},
    {HELP, 0, "h", "help", option::Arg::None,
        "  -h, --help            show this help message and exit"},
    {DOMAIN, 0, "d", "domain", option::Arg::Optional,
        "  -d, --domain=ID       DDS domain ID (default: 0)"},
    {TIMEOUT, 0, "t", "timeout", option::Arg::Optional,
        "  -t, --timeout=SEC     discovery timeout in seconds (default: 2.0)"},
    {TOPIC_FILTER, 0, "f", "filter", option::Arg::Optional,
        "  -f, --filter=TOPIC    filter by topic name"},
    {SHOW_SELF, 0, "", "show-self", option::Arg::None,
        "  --show-self           show this tool's own participant"},
    {VERBOSE, 0, "v", "verbose", option::Arg::None,
        "  -v, --verbose         show detailed QoS properties"},
    {0, 0, nullptr, nullptr, nullptr, nullptr}
};

struct ParticipantInfo {
    core::Guid guid;
    std::string hostname;
    std::string process_name;
    std::string pid;
    std::string entity_name;
    std::set<std::string> pub_topics;
    std::set<std::string> sub_topics;
};

std::string get_qos_prop(const dds_qos_t* qos, const char* name) {
    char* value = nullptr;
    if (dds_qget_prop(qos, name, &value) && value) {
        std::string result(value);
        dds_free(value);
        return result;
    }
    return {};
}

std::string get_qos_entity_name(const dds_qos_t* qos) {
    char* name = nullptr;
    if (dds_qget_entity_name(qos, &name) && name) {
        std::string result(name);
        dds_free(name);
        return result;
    }
    return {};
}

}  // anonymous namespace

int PsCommand::execute(int argc, char* argv[]) {
    g_ps_running.store(true);

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

    int32_t domain_id = 0;
    double timeout_sec = 2.0;
    bool show_self = options[SHOW_SELF];
    bool verbose = options[VERBOSE];
    std::string topic_filter;

    if (options[DOMAIN] && options[DOMAIN].arg) {
        domain_id = std::stoi(options[DOMAIN].arg);
    }
    if (options[TIMEOUT] && options[TIMEOUT].arg) {
        timeout_sec = std::stod(options[TIMEOUT].arg);
    }
    if (options[TOPIC_FILTER] && options[TOPIC_FILTER].arg) {
        topic_filter = options[TOPIC_FILTER].arg;
    }

    // Create participant
    dds::Participant::Config part_config;
    part_config.domain_id = domain_id;
    part_config.participant_name = "cddsctl_ps";
    part_config.enable_topic_discovery = true;

    dds::Participant participant(part_config);
    if (!participant.is_valid()) {
        std::cerr << "Error: Failed to create DDS participant on domain "
                  << domain_id << std::endl;
        return 1;
    }

    dds_guid_t self_guid = participant.guid();

    // Create builtin topic readers
    dds_entity_t participant_reader = dds_create_reader(
        participant.handle(), DDS_BUILTIN_TOPIC_DCPSPARTICIPANT, nullptr, nullptr);
    dds_entity_t publication_reader = dds_create_reader(
        participant.handle(), DDS_BUILTIN_TOPIC_DCPSPUBLICATION, nullptr, nullptr);
    dds_entity_t subscription_reader = dds_create_reader(
        participant.handle(), DDS_BUILTIN_TOPIC_DCPSSUBSCRIPTION, nullptr, nullptr);

    if (participant_reader < 0 || publication_reader < 0 || subscription_reader < 0) {
        std::cerr << "Error: Failed to create builtin topic readers" << std::endl;
        return 1;
    }

    std::mutex info_mutex;
    // Key: participant GUID prefix (first 12 bytes)
    std::map<core::Guid, ParticipantInfo> participants;

    auto get_participant_guid = [](const dds_guid_t& endpoint_guid) -> core::Guid {
        // Participant GUID = first 12 bytes of endpoint GUID + {0,0,1,0xc1}
        dds_guid_t pguid;
        std::memcpy(&pguid, &endpoint_guid, 12);
        pguid.v[12] = 0;
        pguid.v[13] = 0;
        pguid.v[14] = 1;
        pguid.v[15] = 0xc1;
        return core::Guid::from_dds_guid(&pguid);
    };

    std::signal(SIGINT, ps_signal_handler);
    std::signal(SIGTERM, ps_signal_handler);

    // Discovery loop
    auto start = std::chrono::steady_clock::now();
    auto timeout = std::chrono::milliseconds(static_cast<int64_t>(timeout_sec * 1000));

    constexpr size_t MAX_SAMPLES = 32;
    void* samples[MAX_SAMPLES];
    dds_sample_info_t infos[MAX_SAMPLES];
    for (size_t i = 0; i < MAX_SAMPLES; ++i) samples[i] = nullptr;

    while (g_ps_running.load()) {
        auto elapsed = std::chrono::steady_clock::now() - start;
        if (elapsed >= timeout) break;

        // Read DCPSParticipant
        dds_return_t n = dds_take(participant_reader, samples, infos, MAX_SAMPLES, MAX_SAMPLES);
        if (n > 0) {
            for (int32_t i = 0; i < n; ++i) {
                if (!infos[i].valid_data || infos[i].instance_state != DDS_IST_ALIVE)
                    continue;
                auto* sample = static_cast<dds_builtintopic_participant_t*>(samples[i]);
                core::Guid guid = core::Guid::from_dds_guid(&sample->key);

                std::lock_guard<std::mutex> lock(info_mutex);
                auto& info = participants[guid];
                info.guid = guid;

                if (sample->qos) {
                    info.hostname = get_qos_prop(sample->qos, "__Hostname");
                    info.process_name = get_qos_prop(sample->qos, "__ProcessName");
                    info.pid = get_qos_prop(sample->qos, "__Pid");
                    info.entity_name = get_qos_entity_name(sample->qos);
                }
            }
            dds_return_loan(participant_reader, samples, n);
        }

        // Read DCPSPublication
        n = dds_take(publication_reader, samples, infos, MAX_SAMPLES, MAX_SAMPLES);
        if (n > 0) {
            for (int32_t i = 0; i < n; ++i) {
                if (!infos[i].valid_data || infos[i].instance_state != DDS_IST_ALIVE)
                    continue;
                auto* sample = static_cast<dds_builtintopic_endpoint_t*>(samples[i]);
                if (!sample->topic_name || std::strncmp(sample->topic_name, "DCPS", 4) == 0)
                    continue;

                core::Guid pguid = get_participant_guid(sample->participant_key);

                std::lock_guard<std::mutex> lock(info_mutex);
                participants[pguid].pub_topics.insert(sample->topic_name);
            }
            dds_return_loan(publication_reader, samples, n);
        }

        // Read DCPSSubscription
        n = dds_take(subscription_reader, samples, infos, MAX_SAMPLES, MAX_SAMPLES);
        if (n > 0) {
            for (int32_t i = 0; i < n; ++i) {
                if (!infos[i].valid_data || infos[i].instance_state != DDS_IST_ALIVE)
                    continue;
                auto* sample = static_cast<dds_builtintopic_endpoint_t*>(samples[i]);
                if (!sample->topic_name || std::strncmp(sample->topic_name, "DCPS", 4) == 0)
                    continue;

                core::Guid pguid = get_participant_guid(sample->participant_key);

                std::lock_guard<std::mutex> lock(info_mutex);
                participants[pguid].sub_topics.insert(sample->topic_name);
            }
            dds_return_loan(subscription_reader, samples, n);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // Cleanup readers
    dds_delete(participant_reader);
    dds_delete(publication_reader);
    dds_delete(subscription_reader);

    // Filter and display
    core::Guid self_core_guid = core::Guid::from_dds_guid(&self_guid);

    // Collect matching participants
    std::vector<const ParticipantInfo*> results;
    {
        std::lock_guard<std::mutex> lock(info_mutex);
        for (const auto& [guid, info] : participants) {
            if (!show_self && guid == self_core_guid) continue;

            if (!topic_filter.empty()) {
                bool has_topic = false;
                for (const auto& t : info.pub_topics) {
                    if (t.find(topic_filter) != std::string::npos) { has_topic = true; break; }
                }
                if (!has_topic) {
                    for (const auto& t : info.sub_topics) {
                        if (t.find(topic_filter) != std::string::npos) { has_topic = true; break; }
                    }
                }
                if (!has_topic) continue;
            }

            results.push_back(&info);
        }
    }

    if (results.empty()) {
        std::cout << "No participants found on domain " << domain_id << std::endl;
        return 0;
    }

    for (const auto* info : results) {
        // Header line: process info or GUID
        if (!info->process_name.empty() || !info->hostname.empty()) {
            std::cout << "\033[1m";  // bold
            if (!info->process_name.empty()) std::cout << info->process_name;
            else std::cout << "(unknown)";
            if (!info->pid.empty()) std::cout << " [" << info->pid << "]";
            if (!info->hostname.empty()) std::cout << " @ " << info->hostname;
            std::cout << "\033[0m\n";
        } else if (!info->entity_name.empty()) {
            std::cout << "\033[1m" << info->entity_name << "\033[0m\n";
        } else {
            std::cout << "\033[1m(unknown)\033[0m\n";
        }

        std::cout << "  GUID: " << info->guid.to_string() << "\n";

        if (verbose && !info->entity_name.empty() &&
            !info->process_name.empty()) {
            std::cout << "  Name: " << info->entity_name << "\n";
        }

        if (!info->pub_topics.empty()) {
            std::cout << "  Publishers:  ";
            bool first = true;
            for (const auto& t : info->pub_topics) {
                if (!first) std::cout << ", ";
                std::cout << t;
                first = false;
            }
            std::cout << "\n";
        }

        if (!info->sub_topics.empty()) {
            std::cout << "  Subscribers: ";
            bool first = true;
            for (const auto& t : info->sub_topics) {
                if (!first) std::cout << ", ";
                std::cout << t;
                first = false;
            }
            std::cout << "\n";
        }

        if (info->pub_topics.empty() && info->sub_topics.empty()) {
            std::cout << "  (no endpoints)\n";
        }

        std::cout << "\n";
    }

    std::cout << "Total: " << results.size() << " participant(s) on domain "
              << domain_id << std::endl;

    return 0;
}

}  // namespace cli
}  // namespace cddsctl
