#pragma once

#include <cddsctl/cli/Command.hpp>

namespace cddsctl {
namespace cli {

class PsCommand : public Command {
public:
    const char* name() const override { return "ps"; }
    const char* description() const override { return "Show DDS participants and applications"; }
    int execute(int argc, char* argv[]) override;
};

}  // namespace cli
}  // namespace cddsctl
