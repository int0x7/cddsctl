#include "commands/RecordCommand.hpp"
#include "commands/ListCommand.hpp"
#include "commands/InfoCommand.hpp"
#include "commands/EchoCommand.hpp"
#include "commands/PsCommand.hpp"
#include "commands/HzCommand.hpp"
#include <cddsctl/cli/Command.hpp>

#include <iostream>
#include <cstring>
#include <memory>
#include <unordered_map>
#include <functional>

namespace {

constexpr const char* VERSION = CDDSCTL_VERSION;

constexpr const char* SHM_STATUS = "with SHM";

// ANSI color codes
namespace color {
    constexpr const char* RESET   = "\033[0m";
    constexpr const char* BOLD    = "\033[1m";
    constexpr const char* CYAN    = "\033[36m";
    constexpr const char* MAGENTA = "\033[35m";
    constexpr const char* YELLOW  = "\033[33m";
    constexpr const char* GREEN   = "\033[32m";
    constexpr const char* DIM     = "\033[2m";
}

void print_banner() {
    std::cout << color::BOLD << color::CYAN << R"(
    _____ _____  _____   _____  _____ _______ _
   / ____|  __ \|  __ \ / ____|/ ____|__   __| |
  | |    | |  | | |  | | (___ | |       | |  | |
  | |    | |  | | |  | |\___ \| |       | |  | |
  | |____| |__| | |__| |____) | |____   | |  | |____
   \_____|_____/|_____/|_____/ \_____|  |_|  |______|
)" << color::RESET << "\n"
       << color::MAGENTA << "  ╔══════════════════════════════════════════╗\n"
       << "  ║" << color::BOLD << color::YELLOW << "   Cyclone DDS Command Line Tool" << color::RESET << color::MAGENTA << "         ║\n"
       << "  ╚══════════════════════════════════════════╝" << color::RESET << "\n";
}

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
        {"ps",     {[]() { return std::make_unique<cddsctl::cli::PsCommand>(); },
                    "Show DDS participants and applications"}},
        {"hz",     {[]() { return std::make_unique<cddsctl::cli::HzCommand>(); },
                    "Display publishing frequency of a DDS topic"}},
    };
    return commands;
}

void print_usage(const char* program_name) {
    print_banner();

    std::cout << "\n  " << color::DIM << "Version: " << color::RESET << color::GREEN << VERSION
              << " (" << SHM_STATUS << ")" << color::RESET << "\n\n"
              << "  " << color::BOLD << "Usage:" << color::RESET << " " << program_name << " <command> [options]\n\n"
              << "  " << color::BOLD << "Commands:" << color::RESET << "\n";

    for (const auto& [name, entry] : get_commands()) {
        const char* status = entry.factory ? "" : " (coming soon)";
        std::cout << "    " << color::CYAN << name << color::RESET
                  << "\t  " << color::DIM << entry.description << status << color::RESET << "\n";
    }

    std::cout << "\n  " << color::DIM << "Run '" << color::RESET << program_name
              << " <command> --help" << color::DIM << "' for more information." << color::RESET << "\n\n";
}

void print_version() {
    std::cout << "cddsctl version " << VERSION << " (" << SHM_STATUS << ")\n";
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
