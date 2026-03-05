#pragma once

#include <cddsctl/cli/Command.hpp>

namespace cddsctl {
namespace cli {

/**
 * @brief Echo command - prints messages from a DDS topic
 */
class EchoCommand : public Command {
public:
    const char* name() const override { return "echo"; }
    const char* description() const override { return "Print messages from a DDS topic"; }
    int execute(int argc, char* argv[]) override;
};

}  // namespace cli
}  // namespace cddsctl
