#include "HzCommand.hpp"
#include <cddsctl/dds/Participant.hpp>
#include <cddsctl/dds/TopicDiscovery.hpp>
#include <cddsctl/dds/RawDataReader.hpp>
#include <cddsctl/dds/TypeSupport.hpp>
#include <cddsctl/core/Log.hpp>

#include <dds/ddsi/ddsi_serdata.h>
#include <dds/ddsi/ddsi_serdata_default.h>

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
#include <deque>
#include <cmath>
#include <map>
#include <algorithm>

namespace cddsctl {
namespace cli {

// Signal handler for graceful shutdown
static std::atomic<bool> g_hz_running{true};

static void hz_signal_handler(int) {
    g_hz_running.store(false);
}

namespace {

enum OptionIndex {
    UNKNOWN,
    HELP,
    DOMAIN,
    WINDOW,
    TIMEOUT
};

const option::Descriptor usage[] = {
    {UNKNOWN, 0, "", "", option::Arg::None,
        "Usage: cddsctl hz [options] /topic_0 [/topic_1 [topic_2 [..]]]\n\n"
        "Display publishing frequency of DDS topics.\n\n"
        "Options:"},
    {HELP, 0, "h", "help", option::Arg::None,
        "  -h, --help            show this help message and exit"},
    {DOMAIN, 0, "d", "domain", option::Arg::Optional,
        "  -d, --domain=ID       DDS domain ID (default: 0)"},
    {WINDOW, 0, "w", "window", option::Arg::Optional,
        "  -w, --window=N        window size for rate calculation (default: 100)"},
    {TIMEOUT, 0, "t", "timeout", option::Arg::Optional,
        "  -t, --timeout=SEC     discovery timeout in seconds (default: 2.0)"},
    {0, 0, nullptr, nullptr, nullptr, nullptr}
};

/**
 * @brief Statistics calculator for message frequency
 */
class FrequencyStats {
public:
    explicit FrequencyStats(size_t window_size = 100)
        : window_size_(window_size) {}

    void add_timestamp(double timestamp_sec) {
        std::lock_guard<std::mutex> lock(mutex_);

        if (!timestamps_.empty() && timestamp_sec <= timestamps_.back()) {
            // Ignore out-of-order or duplicate timestamps
            return;
        }

        timestamps_.push_back(timestamp_sec);

        // Maintain window size
        while (timestamps_.size() > window_size_) {
            timestamps_.pop_front();
        }
    }

    bool has_data() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return timestamps_.size() >= 2;
    }

    size_t count() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return timestamps_.size();
    }

    struct Stats {
        double avg_hz = 0.0;
        double min_hz = 0.0;
        double max_hz = 0.0;
        double stddev_hz = 0.0;
        size_t window = 0;
    };

    Stats calculate() const {
        std::lock_guard<std::mutex> lock(mutex_);
        Stats result;

        if (timestamps_.size() < 2) {
            return result;
        }

        // Calculate intervals between consecutive messages
        std::vector<double> intervals;
        intervals.reserve(timestamps_.size() - 1);

        for (size_t i = 1; i < timestamps_.size(); ++i) {
            double interval = timestamps_[i] - timestamps_[i - 1];
            if (interval > 0) {
                intervals.push_back(interval);
            }
        }

        if (intervals.empty()) {
            return result;
        }

        // Calculate average interval
        double sum_intervals = 0.0;
        double min_interval = intervals[0];
        double max_interval = intervals[0];

        for (double interval : intervals) {
            sum_intervals += interval;
            min_interval = std::min(min_interval, interval);
            max_interval = std::max(max_interval, interval);
        }

        double avg_interval = sum_intervals / intervals.size();

        // Calculate standard deviation of intervals
        double sum_sq_diff = 0.0;
        for (double interval : intervals) {
            double diff = interval - avg_interval;
            sum_sq_diff += diff * diff;
        }
        double stddev_interval = std::sqrt(sum_sq_diff / intervals.size());

        // Convert to Hz (frequency)
        result.avg_hz = 1.0 / avg_interval;
        result.min_hz = 1.0 / max_interval;  // max interval = min frequency
        result.max_hz = 1.0 / min_interval;  // min interval = max frequency
        result.stddev_hz = stddev_interval * result.avg_hz * result.avg_hz;  // approximate
        result.window = timestamps_.size();

        return result;
    }

private:
    mutable std::mutex mutex_;
    std::deque<double> timestamps_;
    size_t window_size_;
};

/**
 * @brief Topic tracking data
 */
struct TopicTracker {
    std::string name;
    std::unique_ptr<dds::RawDataReader> reader;
    FrequencyStats stats;
    bool found = false;

    TopicTracker(const std::string& topic_name, size_t window_size)
        : name(topic_name), stats(window_size) {}
};

/**
 * @brief Take CDR samples from reader and record timestamps
 * @param reader DDS reader entity handle
 * @param stats FrequencyStats to update
 * @return Number of samples recorded
 */
size_t take_and_record(dds_entity_t reader, FrequencyStats& stats) {
    constexpr size_t MAX_SAMPLES = 32;
    dds_sample_info_t infos[MAX_SAMPLES];
    struct ddsi_serdata* serdatas[MAX_SAMPLES];

    dds_return_t n = dds_takecdr(reader, serdatas, MAX_SAMPLES, infos, DDS_ANY_STATE);
    if (n <= 0) {
        return 0;
    }

    size_t recorded = 0;
    auto now = std::chrono::steady_clock::now();
    double now_sec = std::chrono::duration<double>(now.time_since_epoch()).count();

    for (int32_t i = 0; i < n; ++i) {
        if (!infos[i].valid_data) {
            ddsi_serdata_unref(serdatas[i]);
            continue;
        }

        // Use source timestamp if available, otherwise use current time
        double timestamp_sec;
        if (infos[i].source_timestamp > 0) {
            timestamp_sec = infos[i].source_timestamp / 1e9;  // Convert from nanoseconds
        } else {
            timestamp_sec = now_sec;
        }

        stats.add_timestamp(timestamp_sec);
        ddsi_serdata_unref(serdatas[i]);
        ++recorded;
    }

    return recorded;
}

void print_topic_stats(const std::string& topic_name, const FrequencyStats::Stats& stats,
                       bool show_name) {
    if (show_name) {
        std::cout << "[" << topic_name << "]\n";
    }

    std::cout << "average rate: " << std::fixed << std::setprecision(3) << stats.avg_hz << " Hz\n"
              << "    min: " << stats.min_hz << " Hz\n"
              << "    max: " << stats.max_hz << " Hz\n"
              << "    std dev: " << stats.stddev_hz << " Hz\n"
              << "    window: " << stats.window << std::endl;
}

}  // anonymous namespace

int HzCommand::execute(int argc, char* argv[]) {
    // Reset global state
    g_hz_running.store(true);

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
    size_t window_size = 100;
    double timeout_sec = 2.0;

    if (options[DOMAIN] && options[DOMAIN].arg) {
        domain_id = std::stoi(options[DOMAIN].arg);
    }

    if (options[WINDOW] && options[WINDOW].arg) {
        window_size = std::stoul(options[WINDOW].arg);
        if (window_size < 2) {
            window_size = 2;
        }
    }

    if (options[TIMEOUT] && options[TIMEOUT].arg) {
        timeout_sec = std::stod(options[TIMEOUT].arg);
    }

    // Collect all non-option arguments (topic names)
    std::vector<std::string> topic_names;
    for (int i = 0; i < argc; ++i) {
        const char* a = argv[i];
        // Skip options and their arguments
        if (a[0] == '-') {
            // If this option has no '=', it may have a separate argument
            if (std::strchr(a, '=') == nullptr) {
                bool is_flag = false;
                const char* flags[] = {"-h", "--help", nullptr};
                for (const char** f = flags; *f; ++f) {
                    if (std::strcmp(a, *f) == 0) { is_flag = true; break; }
                }
                if (!is_flag) { ++i; }  // Skip the argument for -d/-w/-t
            }
            continue;
        }
        topic_names.push_back(a);
    }

    // Check for topic names
    if (topic_names.empty()) {
        std::cerr << "Error: at least one topic name is required\n\n";
        option::printUsage(std::cerr, usage);
        return 1;
    }

    // Create participant
    dds::Participant::Config part_config;
    part_config.domain_id = domain_id;
    part_config.participant_name = "cddsctl_hz";
    part_config.enable_topic_discovery = true;

    dds::Participant participant(part_config);
    if (!participant.is_valid()) {
        std::cerr << "Error: Failed to create DDS participant on domain "
                  << domain_id << std::endl;
        return 1;
    }

    // Create trackers for each topic
    std::mutex trackers_mutex;
    std::vector<std::unique_ptr<TopicTracker>> trackers;
    for (const auto& name : topic_names) {
        trackers.push_back(std::make_unique<TopicTracker>(name, window_size));
    }

    // Create topic discovery
    dds::TopicDiscovery discovery(participant);

    discovery.set_endpoint_discovered_callback(
        [&](const dds::DiscoveredEndpoint& ep) {
            std::lock_guard<std::mutex> lock(trackers_mutex);

            for (auto& tracker : trackers) {
                if (tracker->found || tracker->reader) {
                    continue;  // Already found
                }

                if (ep.topic_name != tracker->name) {
                    continue;
                }

                // Create reader in callback - type_info is only valid here
                dds::RawDataReader::Config cfg;
                cfg.topic_name = ep.topic_name;
                cfg.type_name = ep.type_name;
                cfg.type_info = ep.type_info;
                cfg.reliability = core::ReliabilityKind::BestEffort;
                cfg.durability = core::DurabilityKind::Volatile;
                cfg.history_depth = window_size;

                tracker->reader = std::make_unique<dds::RawDataReader>(participant, cfg);
                if (tracker->reader && tracker->reader->is_valid()) {
                    tracker->found = true;
                } else {
                    tracker->reader.reset();
                }
                break;  // Found the matching tracker
            }
        });

    // Setup signal handler for Ctrl+C
    std::signal(SIGINT, hz_signal_handler);
    std::signal(SIGTERM, hz_signal_handler);

    // Start discovery
    discovery.start();

    // Wait for all topics discovery or timeout
    auto start = std::chrono::steady_clock::now();
    auto timeout = std::chrono::milliseconds(static_cast<int64_t>(timeout_sec * 1000));

    while (g_hz_running.load()) {
        {
            std::lock_guard<std::mutex> lock(trackers_mutex);
            bool all_found = true;
            for (const auto& tracker : trackers) {
                if (!tracker->found) {
                    all_found = false;
                    break;
                }
            }
            if (all_found) {
                break;
            }
        }

        auto elapsed = std::chrono::steady_clock::now() - start;
        if (elapsed >= timeout) {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    // Check which topics were found
    std::vector<TopicTracker*> found_trackers;
    {
        std::lock_guard<std::mutex> lock(trackers_mutex);
        for (auto& tracker : trackers) {
            if (tracker->found) {
                found_trackers.push_back(tracker.get());
            }
        }
    }

    if (found_trackers.empty()) {
        std::cout << "No topics found on domain " << domain_id << std::endl;
        discovery.stop();
        return 1;
    }

    // Print subscribed messages
    for (const auto* tracker : found_trackers) {
        std::cout << "subscribed to [" << tracker->name << "]" << std::endl;
    }

    // Main loop: read messages and update stats
    auto last_print_time = std::chrono::steady_clock::now();
    size_t num_topics = found_trackers.size();

    while (g_hz_running.load()) {
        // Read from all readers
        {
            std::lock_guard<std::mutex> lock(trackers_mutex);
            for (auto* tracker : found_trackers) {
                if (tracker->reader && tracker->reader->is_valid()) {
                    take_and_record(tracker->reader->handle(), tracker->stats);
                }
            }
        }

        // Print stats periodically (every 1 second)
        auto now = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration<double>(now - last_print_time).count();

        if (elapsed >= 1.0) {
            bool any_data = false;
            for (const auto* tracker : found_trackers) {
                if (tracker->stats.has_data()) {
                    any_data = true;
                    break;
                }
            }

            if (any_data) {
                // Print stats for all topics (like rostopic hz - each update is a new block)
                for (auto* tracker : found_trackers) {
                    if (tracker->stats.has_data()) {
                        FrequencyStats::Stats s = tracker->stats.calculate();
                        print_topic_stats(tracker->name, s, num_topics > 1);
                    }
                }
                std::cout << "---" << std::endl;
                last_print_time = now;
            }
        }

        // Small sleep to avoid busy-waiting
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Print final stats
    std::cout << "\n";
    for (auto* tracker : found_trackers) {
        if (tracker->stats.has_data()) {
            FrequencyStats::Stats s = tracker->stats.calculate();
            print_topic_stats(tracker->name, s, num_topics > 1);
        }
    }

    // Cleanup
    discovery.stop();

    return 0;
}

}  // namespace cli
}  // namespace cddsctl
