#pragma once

#include <cddsctl/cli/Command.hpp>

namespace cddsctl {
namespace cli {

/**
 * @brief Record command - records DDS topics to MCAP files
 */
class RecordCommand : public Command {
public:
    const char* name() const override { return "record"; }
    const char* description() const override { return "Record DDS topics to MCAP file"; }
    int execute(int argc, char* argv[]) override;
};

}  // namespace cli
}  // namespace cddsctl
