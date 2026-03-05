#include "RecordCommand.hpp"
#include <cddsctl/record/Recorder.hpp>
#include <cddsctl/record/Configuration.hpp>
#include <cddsctl/core/Log.hpp>

#include <optionparser.h>
#include <iostream>
#include <csignal>
#include <atomic>
#include <cstring>
#include <vector>
#include <thread>

namespace cddsctl {
namespace cli {

// Global recorder pointer for signal handler
static recorder::Recorder* g_recorder = nullptr;
static std::atomic<bool> g_running{true};

static void signal_handler(int signal) {
    std::cout << "\nReceived signal " << signal << ", stopping recording..." << std::endl;
    g_running.store(false);
    if (g_recorder) {
        g_recorder->stop();
    }
}

namespace {

enum OptionIndex {
    UNKNOWN,
    HELP,
    CONFIG,
    ALL,
    REGEX,
    EXCLUDE,
    OUTPUT,
    OUTPUT_NAME,
    DOMAIN,
    SPLIT,
    MAX_SPLITS,
    SIZE,
    DURATION,
    MIN_SPACE,
    ZSTD,
    LZ4,
    NO_COMPRESS,
    QUIET,
    VERBOSE
};

const option::Descriptor usage[] = {
    {UNKNOWN, 0, "", "", option::Arg::None,
        "Usage: cddsctl record [options] [TOPIC1 TOPIC2 ...]\n\n"
        "Record DDS topics to MCAP file.\n\n"
        "Options:"},
    {HELP, 0, "h", "help", option::Arg::None,
        "  -h, --help            show this help message and exit"},
    {CONFIG, 0, "c", "config", option::Arg::Optional,
        "  -c, --config=FILE     configuration file path (YAML)"},
    {ALL, 0, "a", "all", option::Arg::None,
        "  -a, --all             record all topics"},
    {REGEX, 0, "e", "regex", option::Arg::None,
        "  -e, --regex           match topics using regular expressions"},
    {EXCLUDE, 0, "x", "exclude", option::Arg::Optional,
        "  -x EXCLUDE_REGEX, --exclude=EXCLUDE_REGEX\n"
        "                        exclude topics matching the follow regular expression\n"
        "                        (subtracts from -a or regex)"},
    {QUIET, 0, "q", "quiet", option::Arg::None,
        "  -q, --quiet           suppress console output"},
    {OUTPUT, 0, "o", "output-prefix", option::Arg::Optional,
        "  -o PREFIX, --output-prefix=PREFIX\n"
        "                        prepend PREFIX to beginning of bag name (name will\n"
        "                        always end with date stamp)"},
    {OUTPUT_NAME, 0, "O", "output-name", option::Arg::Optional,
        "  -O NAME, --output-name=NAME\n"
        "                        record to file with name NAME.mcap"},
    {DOMAIN, 0, "d", "domain", option::Arg::Optional,
        "  -d, --domain=ID       DDS domain ID (default: 0)"},
    {SPLIT, 0, "", "split", option::Arg::None,
        "  --split               split the file when maximum size or duration is reached"},
    {MAX_SPLITS, 0, "", "max-splits", option::Arg::Optional,
        "  --max-splits=MAX_SPLITS\n"
        "                        Keep a maximum of N files, when reaching the\n"
        "                        maximum erase the oldest one to keep a constant number\n"
        "                        of files."},
    {SIZE, 0, "", "size", option::Arg::Optional,
        "  --size=SIZE           record a file of maximum size SIZE MB. (Default:\n"
        "                        infinite)"},
    {DURATION, 0, "", "duration", option::Arg::Optional,
        "  --duration=DURATION   record a file of maximum duration DURATION in seconds,\n"
        "                        unless 'm', or 'h' is appended."},
    {MIN_SPACE, 0, "L", "min-space", option::Arg::Optional,
        "  -L MIN_SPACE, --min-space=MIN_SPACE\n"
        "                        Minimum allowed space on recording device (use G,M,k\n"
        "                        multipliers)"},
    {ZSTD, 0, "", "zstd", option::Arg::None,
        "  --zstd                use zstd compression (default)"},
    {LZ4, 0, "", "lz4", option::Arg::None,
        "  --lz4                 use LZ4 compression"},
    {NO_COMPRESS, 0, "", "no-compression", option::Arg::None,
        "  --no-compression      disable compression"},
    {VERBOSE, 0, "v", "verbose", option::Arg::None,
        "  -v, --verbose         enable verbose logging"},
    {0, 0, nullptr, nullptr, nullptr, nullptr}
};

// Parse size string with optional G/M/k suffix, returns bytes
uint64_t parse_size(const char* str) {
    char* end = nullptr;
    double val = std::strtod(str, &end);
    if (end && *end) {
        switch (*end) {
            case 'G': case 'g': return static_cast<uint64_t>(val * 1024 * 1024 * 1024);
            case 'M': case 'm': return static_cast<uint64_t>(val * 1024 * 1024);
            case 'k': case 'K': return static_cast<uint64_t>(val * 1024);
            default: break;
        }
    }
    return static_cast<uint64_t>(val * 1024 * 1024);  // default to MB
}

// Parse duration string with optional m/h suffix, returns seconds
double parse_duration(const char* str) {
    char* end = nullptr;
    double val = std::strtod(str, &end);
    if (end && *end) {
        switch (*end) {
            case 'm': return val * 60.0;
            case 'h': return val * 3600.0;
            default: break;
        }
    }
    return val;  // default: seconds
}

}  // anonymous namespace

int RecordCommand::execute(int argc, char* argv[]) {
    // Reset global state for potential re-entry
    g_recorder = nullptr;
    g_running.store(true);

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

    // Load configuration (YAML as base, CLI overrides)
    recorder::Configuration config;

    if (options[CONFIG] && options[CONFIG].arg) {
        try {
            config.load_from_file(options[CONFIG].arg);
            LOG_INFO("Loaded configuration from '{}'", options[CONFIG].arg);
        } catch (const core::ConfigException& e) {
            LOG_ERROR("Failed to load config: {}", e.what());
            return 1;
        }
    }

    // Collect non-option arguments (topics)
    {
        std::vector<std::string> cli_topics;
        for (int i = 0; i < argc; ++i) {
            const char* a = argv[i];
            // Skip options and their arguments
            if (a[0] == '-') {
                if (std::strchr(a, '=') == nullptr) {
                    bool is_flag = false;
                    const char* flags[] = {"-h", "--help", "-a", "--all", "-e", "--regex",
                        "--split", "--zstd", "--lz4", "--no-compression", "-q", "--quiet",
                        "-v", "--verbose", nullptr};
                    for (const char** f = flags; *f; ++f) {
                        if (std::strcmp(a, *f) == 0) { is_flag = true; break; }
                    }
                    if (!is_flag) { ++i; }
                }
                continue;
            }
            // Skip ROS remapping arguments
            if (std::strncmp(a, "__", 2) == 0 && std::strchr(a, ':') != nullptr) {
                continue;
            }
            cli_topics.push_back(a);
        }

        if (options[ALL]) {
            core::TopicFilter filter;
            config.set_topic_filter(filter);
        } else if (!cli_topics.empty()) {
            core::TopicFilter filter;
            filter.topics = std::move(cli_topics);
            filter.use_regex = !!options[REGEX];
            config.set_topic_filter(filter);
        }

        if (options[REGEX] && cli_topics.empty() && !options[ALL]) {
            auto filter = config.dds_config().topic_filter;
            filter.use_regex = true;
            config.set_topic_filter(filter);
        }
    }

    // Handle -x exclude
    if (options[EXCLUDE] && options[EXCLUDE].arg) {
        auto filter = config.dds_config().topic_filter;
        filter.exclude_regex = options[EXCLUDE].arg;
        config.set_topic_filter(filter);
    }

    // Override with command line options
    if (options[OUTPUT_NAME] && options[OUTPUT_NAME].arg) {
        config.set_output_prefix(options[OUTPUT_NAME].arg);
        config.mutable_output_config().append_date = false;
    } else if (options[OUTPUT] && options[OUTPUT].arg) {
        config.set_output_prefix(options[OUTPUT].arg);
    }

    if (options[DOMAIN] && options[DOMAIN].arg) {
        config.set_domain_id(std::stoi(options[DOMAIN].arg));
    }

    if (options[DURATION] && options[DURATION].arg) {
        config.set_max_duration(parse_duration(options[DURATION].arg));
    }

    if (options[SIZE] && options[SIZE].arg) {
        config.set_max_file_size(static_cast<size_t>(parse_size(options[SIZE].arg)));
    }

    if (options[SPLIT]) {
        config.set_split(true);
    }

    if (options[MAX_SPLITS] && options[MAX_SPLITS].arg) {
        config.set_max_splits(static_cast<uint32_t>(std::stoul(options[MAX_SPLITS].arg)));
    }

    if (options[MIN_SPACE] && options[MIN_SPACE].arg) {
        config.set_min_space(parse_size(options[MIN_SPACE].arg));
    }

    if (options[ZSTD]) {
        config.set_compression(true);
        config.set_compression_type("zstd");
    }

    if (options[LZ4]) {
        config.set_compression(true);
        config.set_compression_type("lz4");
    }

    if (options[NO_COMPRESS]) {
        config.set_compression(false);
    }

    // Setup signal handlers
    std::signal(SIGINT, signal_handler);
    std::signal(SIGTERM, signal_handler);

    // Run recorder
    {
        recorder::Recorder dds_recorder(config);
        g_recorder = &dds_recorder;

        if (!dds_recorder.init()) {
            LOG_ERROR("Failed to initialize recorder");
            return 1;
        }

        if (!dds_recorder.start()) {
            LOG_ERROR("Failed to start recording");
            return 1;
        }

        LOG_INFO("Recording to '{}' - Press Ctrl+C to stop", dds_recorder.output_path());

        while (g_running.load() && dds_recorder.is_recording()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        if (dds_recorder.is_recording()) {
            dds_recorder.stop();
        }

        // Print statistics
        const auto& stats = dds_recorder.stats();
        std::cout << "\n=== Recording Statistics ===" << std::endl;
        std::cout << "Topics discovered: " << stats.topics_discovered.load() << std::endl;
        std::cout << "Messages received: " << stats.messages_received.load() << std::endl;
        std::cout << "Messages recorded: " << stats.messages_recorded.load() << std::endl;
        std::cout << "Bytes recorded: " << stats.bytes_recorded.load() << std::endl;
        std::cout << "Duration: " << stats.duration_seconds() << " seconds" << std::endl;
        std::cout << "Output file: " << dds_recorder.output_path() << std::endl;

        g_recorder = nullptr;
    }

    return 0;
}

}  // namespace cli
}  // namespace cddsctl
