#pragma once

#include <dds/dds.h>
#include <dds/ddsi/ddsi_xt_typeinfo.h>
#include <dds/ddsi/ddsi_xt_typemap.h>
#include <dds/ddsi/ddsi_cdrstream.h>

#include <cstring>
#include <string>

namespace cddsctl {
namespace dds {
namespace cdr {

// Read primitive types from CDR stream
template<typename T>
inline T read_primitive(dds_istream_t& is) {
    T value;
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
inline std::string read_string(dds_istream_t& is) {
    uint32_t len = read_primitive<uint32_t>(is);
    if (len == 0 || is.m_index + len > is.m_size) {
        return "";
    }
    std::string result(reinterpret_cast<const char*>(is.m_buffer + is.m_index), len - 1);
    is.m_index += len;
    return result;
}

// Check if a type is primitive
inline bool is_primitive_type(uint8_t type_kind) {
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

// Find struct type by equivalence hash in TypeMapping
inline const DDS_XTypes_CompleteStructType* find_struct_by_hash(
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
inline const DDS_XTypes_CompleteEnumeratedType* find_enum_by_hash(
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

// Find the main struct type in TypeMapping (last struct = top-level type)
inline const DDS_XTypes_CompleteStructType* find_main_struct(
    const DDS_XTypes_TypeMapping& mapping)
{
    const auto& complete_pairs = mapping.identifier_object_pair_complete;
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

// Deserialize TypeMapping from descriptor. Caller must free with
// dds_sample_free(mapping, &DDS_XTypes_TypeMapping_desc, DDS_FREE_ALL)
inline DDS_XTypes_TypeMapping* deserialize_type_mapping(
    const dds_topic_descriptor_t* descriptor)
{
    void* mapping_sample = dds_alloc(DDS_XTypes_TypeMapping_desc.m_size);
    if (!mapping_sample) {
        return nullptr;
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

    return static_cast<DDS_XTypes_TypeMapping*>(mapping_sample);
}

}  // namespace cdr
}  // namespace dds
}  // namespace cddsctl
