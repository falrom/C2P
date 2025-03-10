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

    logger.info(std::string("Project Version: v") + c2p::ProjectVersion);
    logger.info(std::string("Project Git Commit: ") + c2p::ProjectGitCommit);
    logger.info(std::string("Project Git Branch: ") + c2p::ProjectGitBranch);
    logger.info(std::string("Project CMake Time: ") + c2p::ProjectCmakeTime);
    logger.info(std::string("Project Build Time: ") + c2p::ProjectBuildTime);

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
                    { .name = "nums",   .shortName = 'n', .typeTag = c2p::TypeTag::NUMBER, .multiple = true, .description = "Specify a series of numbers." },
                    { .name = "input",  .shortName = 'i', .typeTag = c2p::TypeTag::STRING, .required = true, .description = "Specify input file path."     },
                    { .name = "output",                   .typeTag = c2p::TypeTag::STRING,                   .description = "Specify output file path."    },
                },
                .minPositionalArgNum = 2,
                .maxPositionalArgNum = 6,
                .positionalArgDescription = "Positional arguments are required as inputs.",
            },
            {
                .command = "sub_cmd2",
                .description = "This is another sub command.",
            },
        }
    };
    // clang-format on

    const auto parser = c2p::cli::Parser::constructFrom(cg, logger);

    // clang-format off
    std::cout << "--------------------------------------------------------------------------------" << std::endl;
    std::cout << (*(*parser).getHelp({},             true, logger)) << "\n";
    std::cout << "--------------------------------------------------------------------------------" << std::endl;
    std::cout << (*(*parser).getHelp({ "sub_cmd" },  true, logger)) << "\n";
    std::cout << "--------------------------------------------------------------------------------" << std::endl;
    std::cout << (*(*parser).getHelp({ "sub_cmd2" }, true, logger)) << "\n";
    std::cout << "--------------------------------------------------------------------------------" << std::endl;
    // clang-format on

    const char* const args[] = {
        "root_cmd",    "sub_cmd",  "-l",           "position1", "-n",
        "1e3",         "-hv",      "-n",           "123",       "--input",
        "~/input.ini", "--output", "./output.exe", "position2", "position3",
    };

    const auto tree =
        parser->parse(sizeof(args) / sizeof(args[0]), args, logger);

    std::cout << c2p::json::dump(tree, true, 4) << std::endl;

    // clang-format off
    // Output should be:
    /*
    --------------------------------------------------------------------------------
    Usage:

      root_cmd [-v] [-h]

      This is a CLI parser exapmle.

    Sub Commands:

      sub_cmd
        This is a sub command.

      sub_cmd2
        This is another sub command.

    Flag Arguments:

      -v, --version
        Show version information.

      -h, --help
        Show help information.
    --------------------------------------------------------------------------------
    Usage:

      root_cmd sub_cmd -i <STRING> [-n <NUMBER>] [--output <STRING>] [-v] [-h] [-l] <positionalArg0> <positionalArg1> [positionalArg2...5]

      This is a sub command.

    Flag Arguments:

      -v, --version
        Show version information.

      -h, --help
        Show help information.

      -l, --list
        List all items.

    Required Value Arguments:

      -i, --input <STRING>
        Specify input file path.

    Optional Value Arguments:

      -n, --nums <NUMBER> [multiple as array]
        Specify a series of numbers.

      --output <STRING>
        Specify output file path.

    Positional Arguments:

      Need 2 ~ 6 positional argument(s).

      Positional arguments are required as inputs.
    --------------------------------------------------------------------------------
    Usage:

      root_cmd sub_cmd2

      This is another sub command.
    --------------------------------------------------------------------------------
    {
        "command": "root_cmd",
        "subCommand": {
            "command": "sub_cmd",
            "flagArgs": [
                "list",
                "help",
                "version"
            ],
            "positionalArgs": [
                "position1",
                "position2",
                "position3"
            ],
            "valueArgs": {
                "input": "~/input.ini",
                "nums": [
                    1000,
                    123
                ],
                "output": "./output.exe"
            }
        }
    }
    */
    // clang-format on

    return 0;
}
