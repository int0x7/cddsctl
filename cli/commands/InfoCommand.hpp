#pragma once

#include <cddsctl/cli/Command.hpp>

namespace cddsctl {
namespace cli {

/**
 * @brief Info command - shows information about a DDS topic
 */
class InfoCommand : public Command {
public:
    const char* name() const override { return "info"; }
    const char* description() const override { return "Show information about a DDS topic"; }
    int execute(int argc, char* argv[]) override;
};

}  // namespace cli
}  // namespace cddsctl
