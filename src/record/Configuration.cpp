#include <cddsctl/record/Configuration.hpp>

namespace cddsctl {
namespace recorder {

void Configuration::parse_specific(const YAML::Node& root) {
    if (root["output"]) {
        parse_output_config(root["output"]);
    }
    if (root["timing"]) {
        parse_timing_config(root["timing"]);
    }
}

void Configuration::parse_output_config(const YAML::Node& node) {
    output_config_.prefix = get_optional<std::string>(
        node, "prefix", "recording");

    output_config_.append_date = get_optional<bool>(
        node, "append_date", true);

    // YAML values are in MB, convert to bytes
    constexpr size_t MB = 1024ULL * 1024;

    output_config_.max_file_size = get_optional<size_t>(
        node, "max_file_size", 0) * MB;

    output_config_.max_duration_seconds = get_optional<double>(
        node, "max_duration", 0.0);

    output_config_.split = get_optional<bool>(
        node, "split", false);

    output_config_.max_splits = get_optional<uint32_t>(
        node, "max_splits", 0);

    output_config_.min_space = get_optional<uint64_t>(
        node, "min_space", 1024) * MB;

    output_config_.buffer_size = get_optional<size_t>(
        node, "buffer_size", 1) * MB;

    if (node["compression"]) {
        const auto& comp = node["compression"];
        output_config_.enable_compression = get_optional<bool>(
            comp, "enabled", true);
        output_config_.compression_type = get_optional<std::string>(
            comp, "type", "zstd");
        output_config_.compression_level = get_optional<int>(
            comp, "level", 3);
    }
}

void Configuration::parse_timing_config(const YAML::Node& node) {
    timing_config_.use_sim_time = get_optional<bool>(
        node, "use_sim_time", false);

    timing_config_.start_immediately = get_optional<bool>(
        node, "start_immediately", true);

    timing_config_.start_trigger_topic = get_optional<std::string>(
        node, "start_trigger_topic", "");
}

} // namespace recorder
} // namespace cddsctl
