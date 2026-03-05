#pragma once

#include <cddsctl/cli/Command.hpp>

namespace cddsctl {
namespace cli {

/**
 * @brief List command - lists available DDS topics
 */
class ListCommand : public Command {
public:
    const char* name() const override { return "list"; }
    const char* description() const override { return "List available DDS topics"; }
    int execute(int argc, char* argv[]) override;
};

}  // namespace cli
}  // namespace cddsctl
