#include <cddsctl/dds/YamlPrinter.hpp>
#include <cddsctl/core/Log.hpp>

#include <dds/ddsi/ddsi_xt_typeinfo.h>
#include <dds/ddsi/ddsi_xt_typemap.h>

#include <sstream>
#include <iomanip>
#include <cstring>
#include <vector>

namespace cddsctl {
namespace dds {

namespace {

// Forward declarations
void format_value(
    std::ostream& os,
    dds_istream_t& is,
    const DDS_XTypes_TypeIdentifier& type_id,
    const DDS_XTypes_TypeMapping& mapping,
    int indent);

void format_struct_members(
    std::ostream& os,
    dds_istream_t& is,
    const DDS_XTypes_CompleteStructType& struct_type,
    const DDS_XTypes_TypeMapping& mapping,
    int indent);

// Output indentation
void print_indent(std::ostream& os, int level) {
    for (int i = 0; i < level; ++i) {
        os << "  ";
    }
}

// Read primitive types from CDR stream
template<typename T>
T read_primitive(dds_istream_t& is) {
    T value;
    // Align to type size
    uint32_t align = sizeof(T);
    if (align > 1) {
        uint32_t misalign = is.m_index % align;
        if (misalign != 0) {
            is.m_index += align - misalign;
        }
    }
    if (is.m_index + sizeof(T) <= is.m_size) {
        std::memcpy(&value, is.m_buffer + is.m_index, sizeof(T));
        is.m_index += sizeof(T);
    } else {
        value = T{};
    }
    return value;
}

// Read string from CDR stream (4-byte length + data)
std::string read_string(dds_istream_t& is) {
    uint32_t len = read_primitive<uint32_t>(is);
    if (len == 0 || is.m_index + len > is.m_size) {
        return "";
    }
    std::string result(reinterpret_cast<const char*>(is.m_buffer + is.m_index), len - 1);  // -1 for null terminator
    is.m_index += len;
    return result;
}

// Find struct type by equivalence hash in TypeMapping
const DDS_XTypes_CompleteStructType* find_struct_by_hash(
    const DDS_XTypes_EquivalenceHash& hash,
    const DDS_XTypes_TypeMapping& mapping)
{
    const auto& complete_pairs = mapping.identifier_object_pair_complete;
    for (uint32_t i = 0; i < complete_pairs._length; ++i) {
        const auto& pair = complete_pairs._buffer[i];
        if (pair.type_identifier._d == DDS_XTypes_EK_COMPLETE) {
            if (memcmp(pair.type_identifier._u.equivalence_hash, hash,
                       sizeof(DDS_XTypes_EquivalenceHash)) == 0) {
                const auto& type_obj = pair.type_object;
                if (type_obj._d == DDS_XTypes_EK_COMPLETE &&
                    type_obj._u.complete._d == DDS_XTypes_TK_STRUCTURE) {
                    return &type_obj._u.complete._u.struct_type;
                }
            }
        }
    }
    return nullptr;
}

// Find enum type by equivalence hash in TypeMapping
const DDS_XTypes_CompleteEnumeratedType* find_enum_by_hash(
    const DDS_XTypes_EquivalenceHash& hash,
    const DDS_XTypes_TypeMapping& mapping)
{
    const auto& complete_pairs = mapping.identifier_object_pair_complete;
    for (uint32_t i = 0; i < complete_pairs._length; ++i) {
        const auto& pair = complete_pairs._buffer[i];
        if (pair.type_identifier._d == DDS_XTypes_EK_COMPLETE) {
            if (memcmp(pair.type_identifier._u.equivalence_hash, hash,
                       sizeof(DDS_XTypes_EquivalenceHash)) == 0) {
                const auto& type_obj = pair.type_object;
                if (type_obj._d == DDS_XTypes_EK_COMPLETE &&
                    type_obj._u.complete._d == DDS_XTypes_TK_ENUM) {
                    return &type_obj._u.complete._u.enumerated_type;
                }
            }
        }
    }
    return nullptr;
}

// Format primitive value
void format_primitive(std::ostream& os, dds_istream_t& is, uint8_t type_kind) {
    switch (type_kind) {
        case DDS_XTypes_TK_BOOLEAN: {
            uint8_t val = read_primitive<uint8_t>(is);
            os << (val ? "true" : "false");
            break;
        }
        case DDS_XTypes_TK_BYTE: {
            uint8_t val = read_primitive<uint8_t>(is);
            os << static_cast<int>(val);
            break;
        }
        case DDS_XTypes_TK_CHAR8: {
            char val = read_primitive<char>(is);
            os << "'" << val << "'";
            break;
        }
        case DDS_XTypes_TK_INT16: {
            int16_t val = read_primitive<int16_t>(is);
            os << val;
            break;
        }
        case DDS_XTypes_TK_UINT16: {
            uint16_t val = read_primitive<uint16_t>(is);
            os << val;
            break;
        }
        case DDS_XTypes_TK_INT32: {
            int32_t val = read_primitive<int32_t>(is);
            os << val;
            break;
        }
        case DDS_XTypes_TK_UINT32: {
            uint32_t val = read_primitive<uint32_t>(is);
            os << val;
            break;
        }
        case DDS_XTypes_TK_INT64: {
            int64_t val = read_primitive<int64_t>(is);
            os << val;
            break;
        }
        case DDS_XTypes_TK_UINT64: {
            uint64_t val = read_primitive<uint64_t>(is);
            os << val;
            break;
        }
        case DDS_XTypes_TK_FLOAT32: {
            float val = read_primitive<float>(is);
            os << std::setprecision(6) << val;
            break;
        }
        case DDS_XTypes_TK_FLOAT64: {
            double val = read_primitive<double>(is);
            os << std::setprecision(15) << val;
            break;
        }
        default:
            os << "?";
            break;
    }
}

// Check if a type is primitive
bool is_primitive_type(uint8_t type_kind) {
    switch (type_kind) {
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
            return true;
        default:
            return false;
    }
}

// Format array elements
void format_array(
    std::ostream& os,
    dds_istream_t& is,
    const DDS_XTypes_TypeIdentifier& element_type,
    uint32_t size,
    const DDS_XTypes_TypeMapping& mapping,
    int indent)
{
    os << "\n";
    for (uint32_t i = 0; i < size; ++i) {
        print_indent(os, indent);
        os << "- ";
        // Check if element is a nested struct
        if (element_type._d == DDS_XTypes_EK_COMPLETE) {
            os << "\n";
            const auto* nested_struct = find_struct_by_hash(
                element_type._u.equivalence_hash, mapping);
            if (nested_struct) {
                format_struct_members(os, is, *nested_struct, mapping, indent + 1);
            }
        } else if (is_primitive_type(element_type._d)) {
            format_primitive(os, is, element_type._d);
            os << "\n";
        } else {
            format_value(os, is, element_type, mapping, indent + 1);
        }
    }
}

// Format sequence elements
void format_sequence(
    std::ostream& os,
    dds_istream_t& is,
    const DDS_XTypes_TypeIdentifier& element_type,
    const DDS_XTypes_TypeMapping& mapping,
    int indent)
{
    uint32_t len = read_primitive<uint32_t>(is);
    if (len == 0) {
        os << " []\n";
        return;
    }

    os << "\n";
    for (uint32_t i = 0; i < len; ++i) {
        print_indent(os, indent);
        os << "- ";
        // Check if element is a nested struct
        if (element_type._d == DDS_XTypes_EK_COMPLETE) {
            os << "\n";
            const auto* nested_struct = find_struct_by_hash(
                element_type._u.equivalence_hash, mapping);
            if (nested_struct) {
                format_struct_members(os, is, *nested_struct, mapping, indent + 1);
            }
        } else if (is_primitive_type(element_type._d)) {
            format_primitive(os, is, element_type._d);
            os << "\n";
        } else {
            format_value(os, is, element_type, mapping, indent + 1);
        }
    }
}

// Format a value based on its type identifier
void format_value(
    std::ostream& os,
    dds_istream_t& is,
    const DDS_XTypes_TypeIdentifier& type_id,
    const DDS_XTypes_TypeMapping& mapping,
    int indent)
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
            format_primitive(os, is, type_id._d);
            os << "\n";
            break;

        // String types
        case DDS_XTypes_TK_STRING8:
        case DDS_XTypes_TI_STRING8_SMALL:
        case DDS_XTypes_TI_STRING8_LARGE: {
            std::string str = read_string(is);
            os << str << "\n";
            break;
        }

        // Small sequence (max bound < 256)
        case DDS_XTypes_TI_PLAIN_SEQUENCE_SMALL: {
            const auto& seq = type_id._u.seq_sdefn;
            if (seq.element_identifier) {
                format_sequence(os, is, *seq.element_identifier, mapping, indent);
            } else {
                os << " []\n";
            }
            break;
        }

        // Large sequence
        case DDS_XTypes_TI_PLAIN_SEQUENCE_LARGE: {
            const auto& seq = type_id._u.seq_ldefn;
            if (seq.element_identifier) {
                format_sequence(os, is, *seq.element_identifier, mapping, indent);
            } else {
                os << " []\n";
            }
            break;
        }

        // Small array (total size < 256)
        case DDS_XTypes_TI_PLAIN_ARRAY_SMALL: {
            const auto& arr = type_id._u.array_sdefn;
            uint32_t total_size = 1;
            for (uint32_t i = 0; i < arr.array_bound_seq._length; ++i) {
                total_size *= arr.array_bound_seq._buffer[i];
            }
            if (arr.element_identifier && total_size > 0) {
                format_array(os, is, *arr.element_identifier, total_size, mapping, indent);
            } else {
                os << " []\n";
            }
            break;
        }

        // Large array
        case DDS_XTypes_TI_PLAIN_ARRAY_LARGE: {
            const auto& arr = type_id._u.array_ldefn;
            uint32_t total_size = 1;
            for (uint32_t i = 0; i < arr.array_bound_seq._length; ++i) {
                total_size *= arr.array_bound_seq._buffer[i];
            }
            if (arr.element_identifier && total_size > 0) {
                format_array(os, is, *arr.element_identifier, total_size, mapping, indent);
            } else {
                os << " []\n";
            }
            break;
        }

        // Complete type reference (nested struct, enum, etc.)
        case DDS_XTypes_EK_COMPLETE: {
            const auto& hash = type_id._u.equivalence_hash;

            // Try as struct
            const auto* nested_struct = find_struct_by_hash(hash, mapping);
            if (nested_struct) {
                os << "\n";
                format_struct_members(os, is, *nested_struct, mapping, indent);
                break;
            }

            // Try as enum
            const auto* enum_type = find_enum_by_hash(hash, mapping);
            if (enum_type) {
                // Read enum as int32
                int32_t enum_val = read_primitive<int32_t>(is);

                // Find the enum literal name
                const auto& literals = enum_type->literal_seq;
                bool found = false;
                for (uint32_t i = 0; i < literals._length; ++i) {
                    if (literals._buffer[i].common.value == enum_val) {
                        os << literals._buffer[i].detail.name << "\n";
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    os << enum_val << "\n";
                }
                break;
            }

            // Unknown nested type
            os << "<?>\n";
            break;
        }

        // Minimal type reference
        case DDS_XTypes_EK_MINIMAL:
            os << "<minimal_ref>\n";
            break;

        default:
            os << "<unknown_type:" << static_cast<int>(type_id._d) << ">\n";
            break;
    }
}

// Format struct members
void format_struct_members(
    std::ostream& os,
    dds_istream_t& is,
    const DDS_XTypes_CompleteStructType& struct_type,
    const DDS_XTypes_TypeMapping& mapping,
    int indent)
{
    const auto& members = struct_type.member_seq;

    for (uint32_t i = 0; i < members._length; ++i) {
        const auto& member = members._buffer[i];
        const char* name = member.detail.name;
        const auto& type_id = member.common.member_type_id;

        print_indent(os, indent);
        os << name << ":";

        // For primitive types and strings, output on same line
        if (is_primitive_type(type_id._d) ||
            type_id._d == DDS_XTypes_TK_STRING8 ||
            type_id._d == DDS_XTypes_TI_STRING8_SMALL ||
            type_id._d == DDS_XTypes_TI_STRING8_LARGE) {
            os << " ";
            format_value(os, is, type_id, mapping, indent + 1);
        } else {
            // Complex types start on next line
            format_value(os, is, type_id, mapping, indent + 1);
        }
    }
}

// Find the main struct type in TypeMapping (first struct)
const DDS_XTypes_CompleteStructType* find_main_struct(
    const DDS_XTypes_TypeMapping& mapping)
{
    const auto& complete_pairs = mapping.identifier_object_pair_complete;

    // The main type is typically the last one in the mapping
    // (dependencies come first)
    for (int32_t i = static_cast<int32_t>(complete_pairs._length) - 1; i >= 0; --i) {
        const auto& pair = complete_pairs._buffer[i];
        const auto& type_obj = pair.type_object;

        if (type_obj._d == DDS_XTypes_EK_COMPLETE &&
            type_obj._u.complete._d == DDS_XTypes_TK_STRUCTURE) {
            return &type_obj._u.complete._u.struct_type;
        }
    }
    return nullptr;
}

}  // anonymous namespace

bool YamlPrinter::is_available(const dds_topic_descriptor_t* descriptor) {
    return descriptor &&
           (descriptor->m_flagset & DDS_TOPIC_XTYPES_METADATA) &&
           descriptor->type_mapping.data &&
           descriptor->type_mapping.sz > 0;
}

std::string YamlPrinter::format(
    const uint8_t* cdr_data,
    size_t cdr_len,
    const dds_topic_descriptor_t* descriptor,
    uint32_t xcdr_version)
{
    if (!cdr_data || cdr_len == 0 || !is_available(descriptor)) {
        return "";
    }

    // Deserialize TypeMapping from XCDR2
    DDS_XTypes_TypeMapping* mapping = nullptr;
    void* mapping_sample = dds_alloc(DDS_XTypes_TypeMapping_desc.m_size);
    if (!mapping_sample) {
        return "";
    }
    memset(mapping_sample, 0, DDS_XTypes_TypeMapping_desc.m_size);

    dds_istream_t map_is;
    dds_istream_init(&map_is,
                     descriptor->type_mapping.sz,
                     descriptor->type_mapping.data,
                     CDR_ENC_VERSION_2);
    dds_stream_read(&map_is,
                    static_cast<char*>(mapping_sample),
                    DDS_XTypes_TypeMapping_desc.m_ops);
    dds_istream_fini(&map_is);

    mapping = static_cast<DDS_XTypes_TypeMapping*>(mapping_sample);

    // Find the main struct type
    const auto* main_struct = find_main_struct(*mapping);
    if (!main_struct) {
        dds_sample_free(mapping_sample, &DDS_XTypes_TypeMapping_desc, DDS_FREE_ALL);
        return "";
    }

    // Initialize CDR input stream
    dds_istream_t is;
    dds_istream_init(&is, static_cast<uint32_t>(cdr_len),
                     cdr_data, xcdr_version);

    // Format the struct
    std::ostringstream os;
    format_struct_members(os, is, *main_struct, *mapping, 0);

    dds_istream_fini(&is);
    dds_sample_free(mapping_sample, &DDS_XTypes_TypeMapping_desc, DDS_FREE_ALL);

    return os.str();
}

}  // namespace dds
}  // namespace cddsctl
