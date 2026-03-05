#include <cddsctl/dds/Participant.hpp>
#include <cddsctl/core/Log.hpp>

#include <dds/ddsc/dds_public_qos.h>
#include <cstring>

namespace cddsctl {
namespace dds {

Participant::Participant()
    : Participant(Config{})
{
}

Participant::Participant(const Config& config)
    : config_(config)
{
    create_participant();
}

Participant::~Participant() {
    destroy_participant();
}

Participant::Participant(Participant&& other) noexcept
    : config_(std::move(other.config_))
    , participant_(other.participant_)
{
    other.participant_ = 0;
}

Participant& Participant::operator=(Participant&& other) noexcept {
    if (this != &other) {
        destroy_participant();
        config_ = std::move(other.config_);
        participant_ = other.participant_;
        other.participant_ = 0;
    }
    return *this;
}

dds_guid_t Participant::guid() const {
    dds_guid_t guid;
    std::memset(&guid, 0, sizeof(guid));

    if (is_valid()) {
        dds_return_t ret = dds_get_guid(participant_, &guid);
        if (ret != DDS_RETCODE_OK) {
            LOG_ERROR("Failed to get participant GUID: {}", dds_strretcode(ret));
        }
    }
    return guid;
}

void Participant::create_participant() {
    // Create QoS for the participant
    dds_qos_t* qos = dds_create_qos();
    if (!qos) {
        LOG_ERROR("Failed to create participant QoS");
        return;
    }

    // Set participant name
    dds_qset_userdata(qos,
        config_.participant_name.c_str(),
        config_.participant_name.size());

    // Create the participant
    participant_ = dds_create_participant(config_.domain_id, qos, nullptr);
    dds_delete_qos(qos);

    if (participant_ < 0) {
        LOG_ERROR("Failed to create DDS participant on domain {}: {}",
            config_.domain_id, dds_strretcode(participant_));
        participant_ = 0;
        return;
    }

    LOG_DEBUG("Created DDS participant '{}' on domain {}",
        config_.participant_name, config_.domain_id);
}

void Participant::destroy_participant() {
    if (participant_ > 0) {
        dds_return_t ret = dds_delete(participant_);
        if (ret != DDS_RETCODE_OK) {
            LOG_WARN("Failed to delete participant: {}", dds_strretcode(ret));
        } else {
            LOG_DEBUG("Deleted DDS participant");
        }
        participant_ = 0;
    }
}

} // namespace dds
} // namespace cddsctl
