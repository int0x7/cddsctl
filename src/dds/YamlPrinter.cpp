#include <cddsctl/dds/YamlPrinter.hpp>
#include <cddsctl/dds/CdrReader.hpp>
#include <cddsctl/core/Log.hpp>

#include <sstream>
#include <iomanip>

namespace cddsctl {
namespace dds {

namespace {

using namespace cdr;

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

void format_union(
    std::ostream& os,
    dds_istream_t& is,
    const DDS_XTypes_CompleteUnionType& union_type,
    const DDS_XTypes_TypeMapping& mapping,
    int indent);

// Output indentation
void print_indent(std::ostream& os, int level) {
    for (int i = 0; i < level; ++i) {
        os << "  ";
    }
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

        // Small sequence
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

        // Small array
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

            const auto* nested_struct = find_struct_by_hash(hash, mapping);
            if (nested_struct) {
                os << "\n";
                format_struct_members(os, is, *nested_struct, mapping, indent);
                break;
            }

            const auto* enum_type = find_enum_by_hash(hash, mapping);
            if (enum_type) {
                int32_t enum_val = read_primitive<int32_t>(is);
                const auto& literals = enum_type->literal_seq;
                bool found = false;
                for (uint32_t i = 0; i < literals._length; ++i) {
                    if (literals._buffer[i].common.value == enum_val) {
                        os << " " << literals._buffer[i].detail.name << "\n";
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    os << " " << enum_val << "\n";
                }
                break;
            }

            const auto* union_type = find_union_by_hash(hash, mapping);
            if (union_type) {
                format_union(os, is, *union_type, mapping, indent);
                break;
            }

            os << "<?>\n";
            break;
        }

        case DDS_XTypes_EK_MINIMAL:
            os << "<minimal_ref>\n";
            break;

        default:
            os << "<unknown_type:" << static_cast<int>(type_id._d) << ">\n";
            break;
    }
}

// Format union value
void format_union(
    std::ostream& os,
    dds_istream_t& is,
    const DDS_XTypes_CompleteUnionType& union_type,
    const DDS_XTypes_TypeMapping& mapping,
    int indent)
{
    // Read discriminator value
    const auto& disc = union_type.discriminator;
    int32_t disc_val = 0;

    // Determine discriminator type and read value
    uint8_t disc_type = disc.common.type_id._d;
    if (disc_type >= DDS_XTypes_TK_BOOLEAN && disc_type <= DDS_XTypes_TK_INT64) {
        // Boolean, byte, char, short, long, long long
        disc_val = static_cast<int32_t>(read_primitive<int64_t>(is));
    } else if (disc_type >= DDS_XTypes_TK_UINT16 && disc_type <= DDS_XTypes_TK_UINT64) {
        // Unsigned types
        disc_val = static_cast<int32_t>(read_primitive<uint64_t>(is));
    } else if (disc_type == DDS_XTypes_TK_INT32 || disc_type == DDS_XTypes_TK_UINT32) {
        disc_val = read_primitive<int32_t>(is);
    } else {
        // For enums and other types, read as int32
        disc_val = read_primitive<int32_t>(is);
    }

    os << "\n";
    print_indent(os, indent);
    os << "_d: " << disc_val << "\n";

    // Find and format the active member based on discriminator value
    const auto& members = union_type.member_seq;
    bool found = false;

    for (uint32_t i = 0; i < members._length && !found; ++i) {
        const auto& member = members._buffer[i];
        const auto& labels = member.common.label_seq;

        // Check if this member matches the discriminator value
        for (uint32_t j = 0; j < labels._length; ++j) {
            if (labels._buffer[j] == disc_val) {
                print_indent(os, indent);
                os << member.detail.name << ":";

                const auto& member_type = member.common.type_id;
                if (is_primitive_type(member_type._d) ||
                    member_type._d == DDS_XTypes_TK_STRING8 ||
                    member_type._d == DDS_XTypes_TI_STRING8_SMALL ||
                    member_type._d == DDS_XTypes_TI_STRING8_LARGE) {
                    os << " ";
                }
                format_value(os, is, member_type, mapping, indent + 1);
                found = true;
                break;
            }
        }
    }

    if (!found) {
        print_indent(os, indent);
        os << "<unknown discriminator: " << disc_val << ">\n";
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

        if (is_primitive_type(type_id._d) ||
            type_id._d == DDS_XTypes_TK_STRING8 ||
            type_id._d == DDS_XTypes_TI_STRING8_SMALL ||
            type_id._d == DDS_XTypes_TI_STRING8_LARGE) {
            os << " ";
            format_value(os, is, type_id, mapping, indent + 1);
        } else {
            format_value(os, is, type_id, mapping, indent + 1);
        }
    }
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
    uint32_t xcdr_version,
    const char* type_name)
{
    if (!cdr_data || cdr_len == 0 || !is_available(descriptor)) {
        return "";
    }

    DDS_XTypes_TypeMapping* mapping = deserialize_type_mapping(descriptor);
    if (!mapping) {
        return "";
    }

    // Find the main struct - prefer type name lookup if provided
    const DDS_XTypes_CompleteStructType* main_struct = nullptr;
    if (type_name && type_name[0] != '\0') {
        main_struct = find_struct_by_type_name(type_name, *mapping);
    }
    // Fallback to heuristic if type name not provided or not found
    if (!main_struct) {
        main_struct = find_main_struct(*mapping);
    }
    if (!main_struct) {
        dds_sample_free(mapping, &DDS_XTypes_TypeMapping_desc, DDS_FREE_ALL);
        return "";
    }

    dds_istream_t is;
    dds_istream_init(&is, static_cast<uint32_t>(cdr_len),
                     cdr_data, xcdr_version);

    std::ostringstream os;
    format_struct_members(os, is, *main_struct, *mapping, 0);

    dds_istream_fini(&is);
    dds_sample_free(mapping, &DDS_XTypes_TypeMapping_desc, DDS_FREE_ALL);

    return os.str();
}

}  // namespace dds
}  // namespace cddsctl
