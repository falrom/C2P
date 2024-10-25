#include "c2p/cli.hpp"

namespace c2p {
namespace cli {

CommandGroup a = {
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

std::optional<Parser>
Parser::constructFrom(const CommandGroup& cg, const Logger& logger) {
    Parser parser;
    if (parser._constructFrom(cg, {}, logger)) {
        return parser;
    }
    return std::nullopt;
}

bool Parser::_constructFrom(
    const CommandGroup& cg,
    const std::vector<std::string>& preCommands,
    const Logger& logger
) {
    // Generate the prefix command string, for logging.
    // e.g. "git::commit::"
    const std::string preCommandStr = [&preCommands]() {
        std::string str;
        for (const auto& preCommand: preCommands) {
            str += preCommand + "::";
        }
        return str;
    }();

    /// Check if the command name is valid.
    if (cg.command.empty()) {
        logger.error(preCommandStr + "???: Command name cannot be empty.");
        return false;
    }
    _command = cg.command;

    // Generate string of current command, for logging.
    const std::string commandStr = preCommandStr + cg.command;

    /// Record the description.
    _description = cg.description;

    if (cg.MinPositionalArgNum > cg.MaxPositionalArgNum) {
        logger.error(
            commandStr + ": Invalid positional argument number range. "
            + "MinPositionalArgNum=" + std::to_string(cg.MinPositionalArgNum)
            + " > MaxPositionalArgNum=" + std::to_string(cg.MaxPositionalArgNum)
        );
        return false;
    }

    // TODO: flagArgs

    // TODO: valueArgs

    /// Construct sub parsers.
    if (!cg.subCommands.empty()) {
        std::vector<std::string> newPreCommands = preCommands;
        newPreCommands.push_back(cg.command);
        for (const auto& subCommand: cg.subCommands) {
            if (_subParsers.find(subCommand.command) != _subParsers.end()) {
                logger.error(
                    commandStr + ": Command name conflict" + subCommand.command
                );
                return false;
            }
            auto [subParserIter, _] =
                _subParsers.insert({ subCommand.command, Parser() });
            if (!subParserIter->second._constructFrom(
                    subCommand, newPreCommands, logger
                ))
            {
                logger.error(
                    commandStr
                    + ": Failed to construct sub parser: " + subCommand.command
                );
                return false;
            }
        }
    }
    return true;
}

}  // namespace cli
}  // namespace c2p
