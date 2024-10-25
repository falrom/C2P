#include "c2p/cli.hpp"

#include <cassert>

namespace c2p {
namespace cli {

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
    _preCommands = preCommands;
    const std::string preCommandStr = [&preCommands]() {
        std::string str;
        for (const auto& preCommand: preCommands) {
            str += preCommand + "::";
        }
        return str;
    }();

    // Check if the command name is valid.
    if (cg.command.empty()) {
        logger.error(preCommandStr + "???: Command name cannot be empty.");
        return false;
    }
    if (cg.command[0] == '-') {
        logger.error(
            preCommandStr + cg.command + ": Command name cannot start with '-'."
        );
        return false;
    }
    _command = cg.command;

    // Generate string of current command, for logging.
    const std::string commandStr = preCommandStr + _command;

    // Record the description.
    _description = cg.description;

    // Check the positional argument number range.
    if (cg.MinPositionalArgNum > cg.MaxPositionalArgNum) {
        logger.error(
            commandStr
            + ": Invalid positional argument number range. "
              "(MinPositionalArgNum="
            + std::to_string(cg.MinPositionalArgNum) + " > MaxPositionalArgNum="
            + std::to_string(cg.MaxPositionalArgNum) + ")"
        );
        return false;
    }
    _MinPositionalArgNum = cg.MinPositionalArgNum;
    _MaxPositionalArgNum = cg.MaxPositionalArgNum;

    // Record flag arguments.
    _flagArgs = cg.flagArgs;
    for (size_t idx = 0; idx < _flagArgs.size(); ++idx) {
        const auto& flagArg = _flagArgs[idx];

        if (flagArg.name.empty()) {
            logger.error(commandStr + ": Flag argument name cannot be empty.");
            return false;
        }
        if (flagArg.name[0] == '-') {
            logger.error(
                commandStr + ": Flag argument name cannot start with '-'."
            );
            return false;
        }

        if (_flagArgsNameTable.find(flagArg.name) != _flagArgsNameTable.end()) {
            logger.error(
                commandStr + ": Flag argument name conflict: " + flagArg.name
            );
            return false;
        }
        _flagArgsNameTable.insert({ flagArg.name, idx });

        if (flagArg.shortName.has_value()) {
            const char shortName = *flagArg.shortName;
            if (shortName == '-') {
                logger.error(
                    commandStr + ": Flag argument short name cannot be '-'."
                );
                return false;
            }
            if (_flagArgsShortNameTable.find(shortName)
                != _flagArgsShortNameTable.end())
            {
                logger.error(
                    commandStr
                    + ": Flag argument short name conflict: " + shortName
                );
                return false;
            }
            _flagArgsShortNameTable.insert({ shortName, idx });
        }
    }

    // Record value arguments.
    _valueArgs = cg.valueArgs;
    for (size_t idx = 0; idx < _valueArgs.size(); ++idx) {
        const auto& valueArg = _valueArgs[idx];

        if (valueArg.name.empty()) {
            logger.error(commandStr + ": Value argument name cannot be empty.");
            return false;
        }
        if (valueArg.name[0] == '-') {
            logger.error(
                commandStr + ": Value argument name cannot start with '-'."
            );
            return false;
        }

        // Check the default value type.
        if (valueArg.defaultValue.has_value()) {
            if (valueArg.defaultValue->typeTag() != valueArg.typeTag) {
                logger.error(
                    commandStr + ": Default value type mismatch: "
                    + valueArg.name + " (expected: "
                    + std::to_string(size_t(valueArg.typeTag)) + ", actual: "
                    + std::to_string(size_t(valueArg.defaultValue->typeTag()))
                );
                return false;
            }
        }

        if (_valueArgsNameTable.find(valueArg.name)
            != _valueArgsNameTable.end())
        {
            logger.error(
                commandStr + ": Value argument name conflict: " + valueArg.name
            );
            return false;
        }
        // Also not allowed to have the same name as a flag argument.
        if (_flagArgsNameTable.find(valueArg.name) != _flagArgsNameTable.end())
        {
            logger.error(
                commandStr
                + ": Value argument name conflict with flag argument: "
            );
            return false;
        }
        _valueArgsNameTable.insert({ valueArg.name, idx });

        if (valueArg.shortName.has_value()) {
            const char shortName = *valueArg.shortName;
            if (shortName == '-') {
                logger.error(
                    commandStr + ": Value argument short name cannot be '-'."
                );
                return false;
            }
            if (_valueArgsShortNameTable.find(shortName)
                != _valueArgsShortNameTable.end())
            {
                logger.error(
                    commandStr
                    + ": Value argument short name conflict: " + shortName
                );
                return false;
            }
            // Also not allowed to have the same name as a flag argument.
            if (_flagArgsShortNameTable.find(shortName)
                != _flagArgsShortNameTable.end())
            {
                logger.error(
                    commandStr
                    + ": Value argument short name conflict with flag "
                      "argument: "
                );
                return false;
            }
            _valueArgsShortNameTable.insert({ shortName, idx });
        }
    }

    // Construct sub parsers.
    if (!cg.subCommands.empty()) {
        std::vector<std::string> newPreCommands = _preCommands;
        newPreCommands.push_back(_command);
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

ValueTree
Parser::parse(int argc, const char* const argv[], const Logger& logger) {
    if (argc < 1) return ValueTree();
    ValueTree tree;
    if (!_parse(tree, argc, argv, logger)) return ValueTree();
    return tree;
}

bool Parser::_parse(
    ValueTree& tree, int argc, const char* const argv[], const Logger& logger
) {
    auto& object = tree.asObject();
    object["command"] = argv[0];
    if (argc == 1) return true;

    // If is using a sub command.
    const auto firstArg = std::string(argv[1]);
    assert(!firstArg.empty());
    if (firstArg[0] != '-') {
        const auto subParserIter = _subParsers.find(firstArg);
        if (subParserIter != _subParsers.end()) {
            return subParserIter->second._parse(
                object["subCommand"], argc - 1, argv + 1, logger
            );
        }
    }

    // TODO

    return false;
}

}  // namespace cli
}  // namespace c2p
