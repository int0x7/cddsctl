#pragma once

#include <string>

namespace cddsctl {
namespace cli {

/**
 * @brief Base interface for CLI subcommands
 */
class Command {
public:
    virtual ~Command() = default;

    /// Command name (e.g., "record", "echo", "list")
    virtual const char* name() const = 0;

    /// Brief description for help text
    virtual const char* description() const = 0;

    /// Execute the command with given arguments
    /// @param argc Argument count (excluding program name and command name)
    /// @param argv Argument vector (starting after command name)
    /// @return Exit code (0 for success)
    virtual int execute(int argc, char* argv[]) = 0;
};

}  // namespace cli
}  // namespace cddsctl
