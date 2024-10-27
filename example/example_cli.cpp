#include <c2p/cli.hpp>
#include <c2p/json.hpp>
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

    // clang-format off
    c2p::cli::CommandGroup cg = {
        .command = "root_cmd",
        .description = "This is a CLI parser exapmle.",
        .flagArgs = {
            { .name = "version", .shortName = 'v', .description = "Show version information." },
            { .name = "help",    .shortName = 'h', .description = "Show help information."    },
        },
        .subCommands = {
            {
                .command = "sub_cmd",
                .description = "This is a sub command.",
                .flagArgs = {
                    { .name = "version", .shortName = 'v', .description = "Show version information." },
                    { .name = "help",    .shortName = 'h', .description = "Show help information."    },
                    { .name = "list",    .shortName = 'l', .description = "List all items."           },
                },
                .valueArgs = {
                    { .name = "nums",   .shortName = 'n', .typeTag = c2p::TypeTag::NUMBER, .multiple=true,   .description = "Specify a number."         },
                    { .name = "input",  .shortName = 'i', .typeTag = c2p::TypeTag::STRING, .required = true, .description = "Specify input file path."  },
                    { .name = "output",                   .typeTag = c2p::TypeTag::STRING,                   .description = "Specify output file path." },
                },
                .MinPositionalArgNum = 2,
                .MaxPositionalArgNum = 6,
            },
        }
    };
    // clang-format on

    const auto parser = c2p::cli::Parser::constructFrom(cg, logger);

    std::cout << (*(*parser).getHelp({}, true, logger)) << "\n\n";
    std::cout << (*(*parser).getHelp({ "sub_cmd" }, true, logger)) << "\n\n";

    const char* const args[] = {
        "root_cmd",    "sub_cmd",  "-l",           "position1", "-n",
        "1e3",         "-hv",      "-n",           "123",       "--input",
        "~/input.ini", "--output", "./output.exe", "position2",
    };

    const auto tree =
        parser->parse(sizeof(args) / sizeof(args[0]), args, logger);

    std::cout << c2p::json::dump(tree, true) << std::endl;
}
