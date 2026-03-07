#pragma once

#include <dds/dds.h>
#include <dds/ddsi/ddsi_cdrstream.h>
#include <string>
#include <cstdint>

namespace cddsctl {
namespace dds {

/**
 * @brief JSON formatter for CDR data
 *
 * Converts CDR-encoded DDS messages to JSON format using XTypes
 * TypeMapping from the topic descriptor. Output is suitable for
 * piping to tools like jq.
 */
class JsonPrinter {
public:
    static std::string format(
        const uint8_t* cdr_data,
        size_t cdr_len,
        const dds_topic_descriptor_t* descriptor,
        uint32_t xcdr_version = CDR_ENC_VERSION_2);

    static bool is_available(const dds_topic_descriptor_t* descriptor);
};

}  // namespace dds
}  // namespace cddsctl
