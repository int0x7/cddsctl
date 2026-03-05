#include "commands/RecordCommand.hpp"
#include "commands/ListCommand.hpp"
#include "commands/InfoCommand.hpp"
#include "commands/EchoCommand.hpp"
#include <cddsctl/cli/Command.hpp>

#include <iostream>
#include <cstring>
#include <memory>
#include <unordered_map>
#include <functional>

namespace {

constexpr const char* BANNER = R"(
  cddsctl
  Cyclone DDS command-line tool v" VERSION "
)";

constexpr const char* VERSION = "1.0.0";

// Command entry: factory (nullptr = not implemented) + description
struct CommandEntry {
    std::function<std::unique_ptr<cddsctl::cli::Command>()> factory;
    const char* description;
};

// Command registry
const std::unordered_map<std::string, CommandEntry>& get_commands() {
    static const std::unordered_map<std::string, CommandEntry> commands = {
        {"record", {[]() { return std::make_unique<cddsctl::cli::RecordCommand>(); },
                    "Record DDS topics to MCAP file"}},
        {"echo",   {[]() { return std::make_unique<cddsctl::cli::EchoCommand>(); },
                    "Print messages from a DDS topic"}},
        {"list",   {[]() { return std::make_unique<cddsctl::cli::ListCommand>(); },
                    "List available DDS topics"}},
        {"info",   {[]() { return std::make_unique<cddsctl::cli::InfoCommand>(); },
                    "Show information about a DDS topic"}},
    };
    return commands;
}

void print_usage(const char* program_name) {
    std::cout << "\n  cddsctl - Cyclone DDS command-line tool v" << VERSION << "\n\n"
              << "Usage: " << program_name << " <command> [options]\n\n"
              << "Commands:\n";

    for (const auto& [name, entry] : get_commands()) {
        const char* status = entry.factory ? "" : " (coming soon)";
        std::cout << "  " << name << "\t" << entry.description << status << "\n";
    }

    std::cout << "\nRun '" << program_name << " <command> --help' for more information.\n";
}

void print_version() {
    std::cout << "cddsctl version " << VERSION << "\n";
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    const char* command = argv[1];

    // Handle global flags
    if (std::strcmp(command, "--help") == 0 || std::strcmp(command, "-h") == 0) {
        print_usage(argv[0]);
        return 0;
    }

    if (std::strcmp(command, "--version") == 0 || std::strcmp(command, "-V") == 0) {
        print_version();
        return 0;
    }

    // Dispatch to subcommand
    const auto& commands = get_commands();
    auto it = commands.find(command);

    if (it == commands.end()) {
        std::cerr << "Error: Unknown command '" << command << "'\n\n";
        print_usage(argv[0]);
        return 1;
    }

    if (!it->second.factory) {
        std::cerr << "Error: '" << command << "' command is not yet implemented.\n";
        return 1;
    }

    auto cmd = it->second.factory();
    return cmd->execute(argc - 2, argv + 2);
}
