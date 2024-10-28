#include "c2p/cli.hpp"

#include "text_utils.hpp"

#include <cassert>
#include <sstream>

namespace c2p {
namespace cli {

bool Parser::_constructFrom(
    const CommandGroup& cg,
    const std::vector<std::string>& preCommands,
    const Logger& logger
) {
    // Generate the prefix command string, for logging.
    // e.g. "git::commit::"
    _preCommands = preCommands;
    const std::string preCommandStr = [this]() {
        std::string str;
        for (const auto& preCommand: _preCommands) {
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
    if (cg.minPositionalArgNum > cg.maxPositionalArgNum) {
        logger.error(
            commandStr + ": Invalid positional argument number range. (min="
            + std::to_string(cg.minPositionalArgNum)
            + " > max=" + std::to_string(cg.maxPositionalArgNum) + ")"
        );
        return false;
    }
    _minPositionalArgNum = cg.minPositionalArgNum;
    _maxPositionalArgNum = cg.maxPositionalArgNum;
    _positionalArgDescription = cg.positionalArgDescription;

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
                commandStr + ": Flag argument name conflict: \"" + flagArg.name
                + "\""
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
                    commandStr + ": Flag argument short name conflict: '"
                    + shortName + "'"
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
                    commandStr + ": Default value type mismatch: \""
                    + valueArg.name + "\" (expected: "
                    + to_string(valueArg.typeTag) + ", actual: "
                    + to_string(valueArg.defaultValue->typeTag()) + ")"
                );
                return false;
            }
        }

        if (_valueArgsNameTable.find(valueArg.name)
            != _valueArgsNameTable.end())
        {
            logger.error(
                commandStr + ": Value argument name conflict: \""
                + valueArg.name + "\""
            );
            return false;
        }
        // Also not allowed to have the same name as a flag argument.
        if (_flagArgsNameTable.find(valueArg.name) != _flagArgsNameTable.end())
        {
            logger.error(
                commandStr
                + ": Value argument name conflict with flag argument: \""
                + valueArg.name + "\""
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
                    commandStr + ": Value argument short name conflict: '"
                    + shortName + "'"
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
                      "argument: '"
                    + shortName + "'"
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
                    commandStr + ": Command name conflict: \""
                    + subCommand.command + "\""
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
                    commandStr + ": Failed to construct sub parser: \""
                    + subCommand.command + "\""
                );
                return false;
            }
        }
    }
    return true;
}

std::optional<Parser>
Parser::constructFrom(const CommandGroup& cg, const Logger& logger) {
    Parser parser;
    if (parser._constructFrom(cg, {}, logger)) {
        return parser;
    }
    return std::nullopt;
}

static bool _parseValue(
    ValueNode& node,
    TypeTag typeTag,
    const std::string& valueStr,
    const Logger& logger
) {
    switch (typeTag) {
        case TypeTag::NONE: {
            // To low case.
            std::string lowCaseValueStr;
            lowCaseValueStr.reserve(valueStr.size());
            for (const auto& ch: valueStr) {
                lowCaseValueStr += std::tolower(ch);
            }
            if (lowCaseValueStr == "null" || lowCaseValueStr == "none") {
                node = NONE;
                return true;
            } else {
                return false;
            }
        } break;
        case TypeTag::BOOL: {
            // To low case.
            std::string lowCaseValueStr;
            lowCaseValueStr.reserve(valueStr.size());
            if (lowCaseValueStr == "true" || lowCaseValueStr == "yes"
                || lowCaseValueStr == "on" || lowCaseValueStr == "1")
            {
                node = true;
                return true;
            } else if (lowCaseValueStr == "false" || lowCaseValueStr == "no"
                       || lowCaseValueStr == "off" || lowCaseValueStr == "0")
            {
                node = false;
                return true;
            } else {
                return false;
            }
        } break;
        case TypeTag::NUMBER: {
            if (valueStr.empty()) return false;
            uint32_t pos = 0;
            if (valueStr[pos] == '+' || valueStr[pos] == '-') ++pos;
            if (pos >= valueStr.size()) return false;
            if (valueStr[pos] == '0') {
                ++pos;
            } else {
                if (!std::isdigit(valueStr[pos])) {
                    return false;
                }
                while (pos < valueStr.size() && std::isdigit(valueStr[pos])) {
                    ++pos;
                }
            }
            if (pos < valueStr.size() && valueStr[pos] == '.') {
                ++pos;
                if (pos >= valueStr.size() || !std::isdigit(valueStr[pos])) {
                    return false;
                }
                while (pos < valueStr.size() && std::isdigit(valueStr[pos]))
                    ++pos;
            }
            if (pos < valueStr.size()
                && (valueStr[pos] == 'e' || valueStr[pos] == 'E'))
            {
                if (++pos && (valueStr[pos] == '+' || valueStr[pos] == '-')) {
                    ++pos;
                }
                if (pos >= valueStr.size() || !std::isdigit(valueStr[pos])) {
                    return false;
                }
                while (pos < valueStr.size() && std::isdigit(valueStr[pos])) {
                    ++pos;
                }
            }
            if (pos < valueStr.size()) return false;
            node = std::stod(valueStr);
            return true;
        } break;
        case TypeTag::STRING: {
            std::string result;
            uint32_t pos = 0;
            while (pos < valueStr.size()) {
                if (valueStr[pos] == '\\') {
                    ++pos;
                    if (pos >= valueStr.size()) {
                        return false;
                    }
                    switch (valueStr[pos]) {
                        case '"': result.push_back('"'); break;
                        case '\\': result.push_back('\\'); break;
                        case '/': result.push_back('/'); break;
                        case 'b': result.push_back('\b'); break;
                        case 'f': result.push_back('\f'); break;
                        case 'n': result.push_back('\n'); break;
                        case 'r': result.push_back('\r'); break;
                        case 't': result.push_back('\t'); break;
                        case 'u': {
                            auto aheadPos = pos;
                            for (int idx = 0; idx < 4; ++idx) {
                                ++aheadPos;
                                if (aheadPos >= valueStr.size()) {
                                    return false;
                                }
                                if (!std::isxdigit(valueStr[aheadPos])) {
                                    return false;
                                }
                            }
                            ++pos;
                            result += unicodeToUtf8(uint32_t(std::stoul(
                                std::string(valueStr.substr(pos, 4)),
                                nullptr,
                                16
                            )));
                            pos = aheadPos;
                            break;
                        }
                        case 'U': {
                            auto aheadPos = pos;
                            for (int idx = 0; idx < 8; ++idx) {
                                ++aheadPos;
                                if (aheadPos >= valueStr.size()) {
                                    return false;
                                }
                                if (!std::isxdigit(valueStr[aheadPos])) {
                                    return false;
                                }
                            }
                            ++pos;
                            result += unicodeToUtf8(uint32_t(std::stoul(
                                std::string(valueStr.substr(pos, 8)),
                                nullptr,
                                16
                            )));
                            pos = aheadPos;
                            break;
                        }
                        default: {
                            return false;
                        }
                    }
                } else {
                    result.push_back(valueStr[pos]);
                }
                ++pos;
            }
            node = result;
            return true;
        } break;
        default: return false;
    }
}

bool Parser::_parse(
    ValueTree& tree, int argc, const char* const argv[], const Logger& logger
) const {
    auto& object = tree.asObject();
    object["command"] = argv[0];
    if (argc == 1) return true;

    // If is using a sub command.
    // Sub command must be the first argument.
    const auto firstArg = std::string(argv[1]);

    if (!firstArg.empty() && firstArg[0] != '-') {
        const auto subParserIter = _subParsers.find(firstArg);
        if (subParserIter != _subParsers.end()) {
            return subParserIter->second._parse(
                object["subCommand"], argc - 1, argv + 1, logger
            );
        } else {
            // Must be a positional argument, check if it is valid.
            // Just for better error message.
            if (_maxPositionalArgNum == 0) {
                logger.error(
                    "Invalid argument: \"" + firstArg
                    + "\", no sub command matched and positional arguments are "
                      "not required."
                );
                return false;
            }
        }
    }

    // Prefix command string, for logging.
    const std::string preCommandStr = [this]() {
        std::string str;
        for (const auto& preCommand: _preCommands) {
            str += preCommand + "::";
        }
        return str;
    }();
    const std::string commandStr = preCommandStr + _command;

    // Insert argument fields.
    auto& flagArgs = object["flagArgs"].asArray();
    auto& valueArgs = object["valueArgs"].asObject();
    auto& positionalArgs = object["positionalArgs"].asArray();

    for (int argIdx = 1; argIdx < argc; ++argIdx) {
        const auto arg = std::string(argv[argIdx]);

        if (arg.empty() || arg[0] != '-') {
            // Positional argument.
            positionalArgs.emplace_back(arg);
            continue;
        }

        // arg.size >= 1 && arg[0] == '-'

        else if (arg.size() == 1)
        {
            logger.error(
                "Invalid argument, cannot be only a single '-' character."
            );
            return false;
        }

        // arg.size >= 2

        else if (arg[1] == '-')
        {
            if (arg.size() == 2) {
                logger.error(
                    "Invalid argument, cannot be only \"--\" without name."
                );
                return false;
            }

            // arg.size >= 3, should be: "--name"

            const auto name = arg.substr(2);

            const auto flagIter = _flagArgsNameTable.find(name);
            if (flagIter != _flagArgsNameTable.end()) {
                flagArgs.emplace_back(name);
                continue;
            }

            const auto valueIter = _valueArgsNameTable.find(name);
            if (valueIter != _valueArgsNameTable.end()) {
                const auto& valueArg = _valueArgs[valueIter->second];
                ++argIdx;
                if (argIdx >= argc) {
                    logger.error(
                        commandStr + ": Missing value for argument: \"" + name
                        + "\""
                    );
                    return false;
                }
                const auto value = std::string(argv[argIdx]);
                ValueNode node;
                if (!_parseValue(node, valueArg.typeTag, value, logger)) {
                    logger.error(
                        commandStr + ": Failed to parse value for argument: \""
                        + name
                        + "\" (expected type: " + to_string(valueArg.typeTag)
                        + ", actual value: \"" + value + "\")"
                    );
                    return false;
                }
                if (valueArg.multiple) {
                    valueArgs[name].asArray().emplace_back(node);
                } else {
                    valueArgs[name] = node;
                }
                continue;
            }

            logger.error(
                commandStr + ": Unknown argument name: \"" + name + "\""
            );
            return false;
        }

        else
        {
            if (arg.size() == 2) {
                // should be: "-n"
                const char shortName = arg[1];

                const auto flagIter = _flagArgsShortNameTable.find(shortName);
                if (flagIter != _flagArgsShortNameTable.end()) {
                    flagArgs.emplace_back(_flagArgs[flagIter->second].name);
                    continue;
                }

                const auto valueIter = _valueArgsShortNameTable.find(shortName);
                if (valueIter != _valueArgsShortNameTable.end()) {
                    const auto& valueArg = _valueArgs[valueIter->second];
                    ++argIdx;
                    if (argIdx >= argc) {
                        logger.error(
                            commandStr + ": Missing value for argument: '"
                            + shortName + "'"
                        );
                        return false;
                    }
                    const auto value = std::string(argv[argIdx]);
                    ValueNode node;
                    if (!_parseValue(node, valueArg.typeTag, value, logger)) {
                        logger.error(
                            commandStr
                            + ": Failed to parse value for argument: '"
                            + shortName
                            + "' (expected type: " + to_string(valueArg.typeTag)
                            + ", actual value: \"" + value + "\")"
                        );
                        return false;
                    }
                    if (valueArg.multiple) {
                        valueArgs[valueArg.name].asArray().emplace_back(node);
                    } else {
                        valueArgs[valueArg.name] = node;
                    }
                    continue;
                }

                logger.error(
                    commandStr + ": Unknown argument short name: '" + shortName
                    + "'"
                );
                return false;
            }

            // arg.size >= 3, should be: "-nml", multiple flag arguments
            else
            {
                for (size_t idx = 1; idx < arg.size(); ++idx) {
                    const char shortName = arg[idx];
                    const auto flagIter =
                        _flagArgsShortNameTable.find(shortName);
                    if (flagIter == _flagArgsShortNameTable.end()) {
                        logger.error(
                            commandStr + ": Unknown flag argument short name: '"
                            + shortName + "' in argument: \"" + arg + "\""
                        );
                        return false;
                    }
                    flagArgs.emplace_back(_flagArgs[flagIter->second].name);
                }
                continue;
            }
        }
    }

    // Check positional argument number.
    if (positionalArgs.size() < _minPositionalArgNum) {
        logger.error(
            commandStr + ": Too few positional arguments: "
            + std::to_string(positionalArgs.size())
            + " (expected: >= " + std::to_string(_minPositionalArgNum) + ")"
        );
        return false;
    } else if (positionalArgs.size() > _maxPositionalArgNum) {
        logger.error(
            commandStr + ": Too many positional arguments: "
            + std::to_string(positionalArgs.size())
            + " (expected: <= " + std::to_string(_maxPositionalArgNum) + ")"
        );
        return false;
    }

    // Insert default values for value arguments that are not specified, and
    // check if required value arguments are missing.
    for (const auto& valueArg: _valueArgs) {
        if (valueArg.defaultValue.has_value()) {
            if (valueArgs.find(valueArg.name) == valueArgs.end()) {
                if (valueArg.multiple) {
                    valueArgs[valueArg.name].asArray().emplace_back(
                        *valueArg.defaultValue
                    );
                } else {
                    valueArgs[valueArg.name] = *valueArg.defaultValue;
                }
            }
        } else if (valueArg.required) {
            if (valueArgs.find(valueArg.name) == valueArgs.end()) {
                logger.error(
                    commandStr + ": Missing required value argument: \""
                    + valueArg.name + "\""
                );
                return false;
            }
        }
    }

    return true;
}

ValueTree
Parser::parse(int argc, const char* const argv[], const Logger& logger) const {
    if (argc < 1) return ValueTree();
    ValueTree tree;
    if (!_parse(tree, argc, argv, logger)) return ValueTree();
    return tree;
}

std::optional<std::string>
Parser::_getHelp(bool enableAnsiFormat, const Logger& logger) const {

    constexpr auto indent = "  ";

    const auto fmtBold = enableAnsiFormat ? FMT_BOLD : "";
    const auto fmtReset = enableAnsiFormat ? FMT_RESET : "";

    const std::string preCommandStr = [this]() {
        std::string str;
        for (const auto& preCommand: _preCommands) {
            str += preCommand + ' ';
        }
        return str;
    }();
    const std::string commandStr = preCommandStr + _command;

    std::vector<size_t> requiredValueArgIdxs;
    std::vector<size_t> optionalValueArgIdxs;

    for (size_t idx = 0; idx < _valueArgs.size(); ++idx) {
        const auto& valueArg = _valueArgs[idx];
        if (valueArg.required && !valueArg.defaultValue.has_value()) {
            requiredValueArgIdxs.push_back(idx);
        } else {
            optionalValueArgIdxs.push_back(idx);
        }
    }

    std::stringstream help;

    help << fmtBold << "Usage:" << fmtReset;
    help << "\n\n" << indent << commandStr;

    for (const auto& valueArgIdx: requiredValueArgIdxs) {
        const auto& valueArg = _valueArgs[valueArgIdx];
        if (valueArg.shortName) {
            help << " -" << *valueArg.shortName;
        } else {
            help << " --" << valueArg.name;
        }
        help << " <" << to_string(valueArg.typeTag) << ">";
    }

    for (const auto& valueArgIdx: optionalValueArgIdxs) {
        const auto& valueArg = _valueArgs[valueArgIdx];
        if (valueArg.shortName) {
            help << " [-" << *valueArg.shortName;
        } else {
            help << " [--" << valueArg.name;
        }
        help << " <" << to_string(valueArg.typeTag) << ">]";
    }

    for (const auto& flagArg: _flagArgs) {
        if (flagArg.shortName) {
            help << " [-" << *flagArg.shortName << "]";
        } else {
            help << " [--" << flagArg.name << "]";
        }
    }

    for (size_t idx = 0; idx < _minPositionalArgNum; ++idx) {
        help << " <positionalArg" << idx << ">";
    }

    if (_maxPositionalArgNum > _minPositionalArgNum) {
        const uint32_t leftPositionalArgNum =
            _maxPositionalArgNum - _minPositionalArgNum;
        if (leftPositionalArgNum == 1) {
            help << " [positionalArg" << _minPositionalArgNum << "]";
        } else {
            help << " [positionalArg" << _minPositionalArgNum << "..."
                 << (_maxPositionalArgNum - 1) << "]";
        }
    }

    if (_description) {
        help << "\n\n" << indent << *_description;
    }

    // Sub commands
    if (!_subParsers.empty()) {
        help << "\n\n" << fmtBold << "Sub Commands:" << fmtReset;
        for (const auto& [subCommand, subParser]: _subParsers) {
            help << "\n\n" << indent << subCommand;
            if (subParser._description) {
                help << "\n" << indent << indent << *subParser._description;
            }
        }
    }

    // Flag arguments
    if (!_flagArgs.empty()) {
        help << "\n\n" << fmtBold << "Flag Arguments:" << fmtReset;
        for (const auto& flagArg: _flagArgs) {
            help << "\n\n" << indent;
            if (flagArg.shortName) {
                help << "-" << *flagArg.shortName;
                if (flagArg.name.size() > 1) {
                    help << ", ";
                }
            }
            help << "--" << flagArg.name;
            if (flagArg.description) {
                help << "\n" << indent << indent << *flagArg.description;
            }
        }
    }

    // Required value arguments
    if (!requiredValueArgIdxs.empty()) {
        help << "\n\n" << fmtBold << "Required Value Arguments:" << fmtReset;
        for (const auto& valueArgIdx: requiredValueArgIdxs) {
            const auto& valueArg = _valueArgs[valueArgIdx];
            help << "\n\n" << indent;
            if (valueArg.shortName) {
                help << "-" << *valueArg.shortName;
                if (valueArg.name.size() > 1) {
                    help << ", ";
                }
            }
            if (valueArg.name.size() > 1) {
                help << "--" << valueArg.name;
            }
            help << " <" << to_string(valueArg.typeTag) << ">";
            if (valueArg.multiple) {
                help << " [multiple as array]";
            }
            if (valueArg.description) {
                help << "\n" << indent << indent << *valueArg.description;
            }
        }
    }

    // Optional value arguments
    if (!optionalValueArgIdxs.empty()) {
        help << "\n\n" << fmtBold << "Optional Value Arguments:" << fmtReset;
        for (const auto& valueArgIdx: optionalValueArgIdxs) {
            const auto& valueArg = _valueArgs[valueArgIdx];
            help << "\n\n" << indent;
            if (valueArg.shortName) {
                help << "-" << *valueArg.shortName;
                if (valueArg.name.size() > 1) {
                    help << ", ";
                }
            }
            if (valueArg.name.size() > 1) {
                help << "--" << valueArg.name;
            }
            help << " <" << to_string(valueArg.typeTag) << ">";
            if (valueArg.multiple) {
                help << " [multiple as array]";
            }
            if (valueArg.description) {
                help << "\n" << indent << indent << *valueArg.description;
            }
        }
    }

    // Positional arguments
    if (_maxPositionalArgNum != 0) {
        help << "\n\n" << fmtBold << "Positional Arguments:" << fmtReset;
        if (_minPositionalArgNum == _maxPositionalArgNum) {
            help << "\n\n"
                 << indent << "Need " << _minPositionalArgNum
                 << " positional arguments.";
        } else {
            help << "\n\n"
                 << indent << "Need " << _minPositionalArgNum << " ~ "
                 << _maxPositionalArgNum << " positional argument(s).";
        }
        if (_positionalArgDescription) {
            help << "\n\n" << indent << *_positionalArgDescription;
        }
    }

    return help.str();
}

std::optional<std::string> Parser::getHelp(
    const std::vector<std::string>& subCommands,
    bool enableAnsiFormat,
    const Logger& logger
) const {
    const Parser* parser = this;
    for (const auto& subCommand: subCommands) {
        const auto subParserIter = parser->_subParsers.find(subCommand);
        if (subParserIter == parser->_subParsers.end()) {
            logger.error("Unknown sub command: \"" + subCommand + "\"");
            return std::nullopt;
        }
        parser = &subParserIter->second;
    }
    return parser->_getHelp(enableAnsiFormat, logger);
}

}  // namespace cli
}  // namespace c2p
