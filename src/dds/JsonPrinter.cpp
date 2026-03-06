#include <cddsctl/dds/JsonPrinter.hpp>
#include <cddsctl/dds/CdrReader.hpp>

#include <nlohmann/json.hpp>

#include <iomanip>

namespace cddsctl {
namespace dds {

namespace {

using namespace cdr;
using json = nlohmann::json;

// Forward declarations
json build_value(
    dds_istream_t& is,
    const DDS_XTypes_TypeIdentifier& type_id,
    const DDS_XTypes_TypeMapping& mapping);

json build_struct(
    dds_istream_t& is,
    const DDS_XTypes_CompleteStructType& struct_type,
    const DDS_XTypes_TypeMapping& mapping);

// Build JSON from a primitive type
json build_primitive(dds_istream_t& is, uint8_t type_kind) {
    switch (type_kind) {
        case DDS_XTypes_TK_BOOLEAN: {
            uint8_t val = read_primitive<uint8_t>(is);
            return json(val != 0);
        }
        case DDS_XTypes_TK_BYTE: {
            uint8_t val = read_primitive<uint8_t>(is);
            return json(val);
        }
        case DDS_XTypes_TK_CHAR8: {
            char val = read_primitive<char>(is);
            return json(std::string(1, val));
        }
        case DDS_XTypes_TK_INT16:
            return json(read_primitive<int16_t>(is));
        case DDS_XTypes_TK_UINT16:
            return json(read_primitive<uint16_t>(is));
        case DDS_XTypes_TK_INT32:
            return json(read_primitive<int32_t>(is));
        case DDS_XTypes_TK_UINT32:
            return json(read_primitive<uint32_t>(is));
        case DDS_XTypes_TK_INT64:
            return json(read_primitive<int64_t>(is));
        case DDS_XTypes_TK_UINT64:
            return json(read_primitive<uint64_t>(is));
        case DDS_XTypes_TK_FLOAT32:
            return json(read_primitive<float>(is));
        case DDS_XTypes_TK_FLOAT64:
            return json(read_primitive<double>(is));
        default:
            return json(nullptr);
    }
}

// Build JSON array from fixed-size array
json build_array(
    dds_istream_t& is,
    const DDS_XTypes_TypeIdentifier& element_type,
    uint32_t size,
    const DDS_XTypes_TypeMapping& mapping)
{
    json arr = json::array();
    for (uint32_t i = 0; i < size; ++i) {
        arr.push_back(build_value(is, element_type, mapping));
    }
    return arr;
}

// Build JSON array from sequence
json build_sequence(
    dds_istream_t& is,
    const DDS_XTypes_TypeIdentifier& element_type,
    const DDS_XTypes_TypeMapping& mapping)
{
    uint32_t len = read_primitive<uint32_t>(is);
    json arr = json::array();
    for (uint32_t i = 0; i < len; ++i) {
        arr.push_back(build_value(is, element_type, mapping));
    }
    return arr;
}

// Build JSON value from type identifier
json build_value(
    dds_istream_t& is,
    const DDS_XTypes_TypeIdentifier& type_id,
    const DDS_XTypes_TypeMapping& mapping)
{
    switch (type_id._d) {
        // Primitive types
        case DDS_XTypes_TK_BOOLEAN:
        case DDS_XTypes_TK_BYTE:
        case DDS_XTypes_TK_CHAR8:
        case DDS_XTypes_TK_INT16:
        case DDS_XTypes_TK_UINT16:
        case DDS_XTypes_TK_INT32:
        case DDS_XTypes_TK_UINT32:
        case DDS_XTypes_TK_INT64:
        case DDS_XTypes_TK_UINT64:
        case DDS_XTypes_TK_FLOAT32:
        case DDS_XTypes_TK_FLOAT64:
            return build_primitive(is, type_id._d);

        // String types
        case DDS_XTypes_TK_STRING8:
        case DDS_XTypes_TI_STRING8_SMALL:
        case DDS_XTypes_TI_STRING8_LARGE:
            return json(read_string(is));

        // Small sequence
        case DDS_XTypes_TI_PLAIN_SEQUENCE_SMALL: {
            const auto& seq = type_id._u.seq_sdefn;
            if (seq.element_identifier) {
                return build_sequence(is, *seq.element_identifier, mapping);
            }
            return json::array();
        }

        // Large sequence
        case DDS_XTypes_TI_PLAIN_SEQUENCE_LARGE: {
            const auto& seq = type_id._u.seq_ldefn;
            if (seq.element_identifier) {
                return build_sequence(is, *seq.element_identifier, mapping);
            }
            return json::array();
        }

        // Small array
        case DDS_XTypes_TI_PLAIN_ARRAY_SMALL: {
            const auto& arr = type_id._u.array_sdefn;
            uint32_t total_size = 1;
            for (uint32_t i = 0; i < arr.array_bound_seq._length; ++i) {
                total_size *= arr.array_bound_seq._buffer[i];
            }
            if (arr.element_identifier && total_size > 0) {
                return build_array(is, *arr.element_identifier, total_size, mapping);
            }
            return json::array();
        }

        // Large array
        case DDS_XTypes_TI_PLAIN_ARRAY_LARGE: {
            const auto& arr = type_id._u.array_ldefn;
            uint32_t total_size = 1;
            for (uint32_t i = 0; i < arr.array_bound_seq._length; ++i) {
                total_size *= arr.array_bound_seq._buffer[i];
            }
            if (arr.element_identifier && total_size > 0) {
                return build_array(is, *arr.element_identifier, total_size, mapping);
            }
            return json::array();
        }

        // Complete type reference (nested struct, enum, etc.)
        case DDS_XTypes_EK_COMPLETE: {
            const auto& hash = type_id._u.equivalence_hash;

            const auto* nested_struct = find_struct_by_hash(hash, mapping);
            if (nested_struct) {
                return build_struct(is, *nested_struct, mapping);
            }

            const auto* enum_type = find_enum_by_hash(hash, mapping);
            if (enum_type) {
                int32_t enum_val = read_primitive<int32_t>(is);
                const auto& literals = enum_type->literal_seq;
                for (uint32_t i = 0; i < literals._length; ++i) {
                    if (literals._buffer[i].common.value == enum_val) {
                        return json(literals._buffer[i].detail.name);
                    }
                }
                return json(enum_val);
            }

            return json(nullptr);
        }

        default:
            return json(nullptr);
    }
}

// Build JSON object from struct members
json build_struct(
    dds_istream_t& is,
    const DDS_XTypes_CompleteStructType& struct_type,
    const DDS_XTypes_TypeMapping& mapping)
{
    json obj = json::object();
    const auto& members = struct_type.member_seq;

    for (uint32_t i = 0; i < members._length; ++i) {
        const auto& member = members._buffer[i];
        const char* name = member.detail.name;
        const auto& type_id = member.common.member_type_id;
        obj[name] = build_value(is, type_id, mapping);
    }

    return obj;
}

}  // anonymous namespace

bool JsonPrinter::is_available(const dds_topic_descriptor_t* descriptor) {
    return descriptor &&
           (descriptor->m_flagset & DDS_TOPIC_XTYPES_METADATA) &&
           descriptor->type_mapping.data &&
           descriptor->type_mapping.sz > 0;
}

std::string JsonPrinter::format(
    const uint8_t* cdr_data,
    size_t cdr_len,
    const dds_topic_descriptor_t* descriptor,
    uint32_t xcdr_version)
{
    if (!cdr_data || cdr_len == 0 || !is_available(descriptor)) {
        return "";
    }

    DDS_XTypes_TypeMapping* mapping = deserialize_type_mapping(descriptor);
    if (!mapping) {
        return "";
    }

    const auto* main_struct = find_main_struct(*mapping);
    if (!main_struct) {
        dds_sample_free(mapping, &DDS_XTypes_TypeMapping_desc, DDS_FREE_ALL);
        return "";
    }

    dds_istream_t is;
    dds_istream_init(&is, static_cast<uint32_t>(cdr_len),
                     cdr_data, xcdr_version);

    json result = build_struct(is, *main_struct, *mapping);

    dds_istream_fini(&is);
    dds_sample_free(mapping, &DDS_XTypes_TypeMapping_desc, DDS_FREE_ALL);

    return result.dump(2) + "\n";
}

}  // namespace dds
}  // namespace cddsctl
