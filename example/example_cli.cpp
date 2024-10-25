#include <c2p/cli.hpp>
#include <c2p/ini.hpp>
#include <iostream>
#include <optional>
#include <vector>

const c2p::Logger logger{
    // clang-format off
    [](const std::string& logStr) { std::cerr << "Error: "   << logStr << std::endl; },
    [](const std::string& logStr) { std::cout << "Warning: " << logStr << std::endl; },
    [](const std::string& logStr) { std::cout << "Info: "    << logStr << std::endl; },
    // clang-format on
};

int main(int argc, char* argv[]) {

    c2p::cli::CommandGroup a = {
        .command = "git",
        .description = "Git is a free and balabalballablab.",
        .flagArgs = {
            { .name = "version", .shortName = 'v', .description = "Show version information." },
            { .name = "help",    .shortName = 'h', .description = "Show help information."    },
        },
        .valueArgs = {
            { .name = "num", .shortName = 'n', .description = "Specify the command to execute." },
        },
        .MinPositionalArgNum = 0,
        .MaxPositionalArgNum = 0,
        .subCommands = {
            {
                .command = "git",
                .description = "Git is a free and balabalballablab.",
                .flagArgs = {
                    { .name = "version", .shortName = 'v', .description = "Show version information." },
                    { .name = "help",    .shortName = 'h', .description = "Show help information."    },
                },
                .valueArgs = {
                    { .name = "num", .shortName = 'n', .description = "Specify the command to execute." },
                },
                .MinPositionalArgNum = 0,
                .MaxPositionalArgNum = 0,
            },
        }
    };
}
