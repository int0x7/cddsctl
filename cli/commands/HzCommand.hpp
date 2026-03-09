#pragma once

#include <cddsctl/cli/Command.hpp>

namespace cddsctl {
namespace cli {

/**
 * @brief Hz command - displays publishing frequency of a DDS topic
 */
class HzCommand : public Command {
public:
    const char* name() const override { return "hz"; }
    const char* description() const override { return "Display publishing frequency of a DDS topic"; }
    int execute(int argc, char* argv[]) override;
};

}  // namespace cli
}  // namespace cddsctl
