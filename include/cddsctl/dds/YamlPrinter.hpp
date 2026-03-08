#pragma once

#include <dds/dds.h>
#include <dds/ddsi/ddsi_cdrstream.h>
#include <string>
#include <cstdint>

namespace cddsctl {
namespace dds {

/**
 * @brief YAML formatter for CDR data
 *
 * Converts CDR-encoded DDS messages to human-readable YAML format,
 * similar to rostopic echo output. Uses XTypes TypeMapping from
 * the topic descriptor to interpret the binary CDR data.
 */
class YamlPrinter {
public:
    /**
     * @brief Format CDR data as YAML string
     * @param cdr_data CDR data (excluding 4-byte encapsulation header)
     * @param cdr_len Length of CDR data
     * @param descriptor Topic descriptor containing type_mapping
     * @param xcdr_version CDR encoding version (CDR_ENC_VERSION_1 or CDR_ENC_VERSION_2)
     * @param type_name Optional type name to identify the main struct (e.g., "demo::robotics::RobotState")
     * @return YAML formatted string, or empty string if formatting fails
     *
     * If the descriptor doesn't have valid type_mapping data,
     * returns an empty string and the caller should fall back to
     * compact format output.
     *
     * The type_name parameter should be the fully qualified type name of the
     * top-level struct. If not provided, the function will attempt to find
     * the main struct by heuristics which may fail for complex nested types.
     */
    static std::string format(
        const uint8_t* cdr_data,
        size_t cdr_len,
        const dds_topic_descriptor_t* descriptor,
        uint32_t xcdr_version = CDR_ENC_VERSION_2,
        const char* type_name = nullptr);

    /**
     * @brief Check if YAML formatting is available for a descriptor
     * @param descriptor Topic descriptor to check
     * @return true if descriptor has valid type_mapping for YAML formatting
     */
    static bool is_available(const dds_topic_descriptor_t* descriptor);
};

}  // namespace dds
}  // namespace cddsctl
