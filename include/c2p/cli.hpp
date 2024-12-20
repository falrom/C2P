/**
 * @file cli.hpp
 * @brief Command line argument parsing.
 * Based on ValueTree.
 */

#ifndef __C2P_CLI_HPP__
#define __C2P_CLI_HPP__

#include <c2p/common.hpp>
#include <c2p/value_tree.hpp>
#include <map>
#include <optional>

namespace c2p {
namespace cli {

struct FlagArgument {

    /// Name of the argument.
    /// Can be used after "--", for example: "--output".
    /// Should be unique within the same command.
    std::string name;

    /// One character short name of the argument.
    /// Can be used after "-", for example: "-l".
    /// Multiple flag arguments can be combined.
    /// For example: "-lah", equivalent to: "-l -a -h".
    std::optional<char> shortName;

    std::optional<std::string> description;
};

struct ValueArgument {

    /// Name of the argument.
    /// Can be used after "--", for example: "--output".
    /// Should be unique within the same command.
    std::string name;

    /// One character short name of the argument.
    /// Can be used after "-", for example: "-a".
    /// If it is a flag argument, it can be combined.
    /// For example: "-lah", equivalent to: "-l -a -h".
    std::optional<char> shortName;

    /// Type of the argument value.
    TypeTag typeTag;

    std::optional<ValueNode> defaultValue;

    /// If the argument is required.
    /// - If also has a default value, the flag is ignored.
    /// - If also specified as multiple, means at least one is required.
    bool required = false;

    /// If the argument can be used multiple times.
    /// For example: "--input file1.txt --input file2.txt".
    /// Will be stored as an array.
    bool multiple = false;

    std::optional<std::string> description;
};

struct CommandGroup {

    /// Name of the command.
    std::string command;

    /// Description of the command.
    std::optional<std::string> description;

    /// List of flag arguments.
    std::vector<FlagArgument> flagArgs;

    /// List of value arguments.
    std::vector<ValueArgument> valueArgs;

    /// Minimum allowed number of positional arguments.
    uint32_t minPositionalArgNum = 0;

    /// Maximum allowed number of positional arguments.
    uint32_t maxPositionalArgNum = 0;

    /// Description of the positional arguments.
    std::optional<std::string> positionalArgDescription;

    /// List of sub commands.
    std::vector<CommandGroup> subCommands;
};

class Parser
{
  public:

    /// Construct a parser from the command group.
    /// Return std::nullopt if the command group config is invalid.
    static std::optional<Parser>
    constructFrom(const CommandGroup& cg, const Logger& logger = Logger());

  public:

    /// Parse command line arguments into ValueTree.
    /// If the input arguments are invalid, return an empty ValueTree.
    ValueTree parse(
        int argc, const char* const argv[], const Logger& logger = Logger()
    ) const;

    /// Generate help message of specified command group.
    ///
    /// @param[in] subCommands If you want to get the help information of a sub
    /// command, you need to specify the names in hierarchical order.
    /// @return Return std::nullopt if the specified command group does not
    /// exist.
    std::optional<std::string> getHelp(
        const std::vector<std::string>& subCommands = {},
        bool enableAnsiFormat = true,
        const Logger& logger = Logger()
    ) const;

  public:

    Parser(const Parser&) = default;
    Parser(Parser&&) = default;
    Parser& operator=(const Parser&) = default;
    Parser& operator=(Parser&&) = default;
    ~Parser() = default;

  private:

    std::string _command;
    std::optional<std::string> _description;

    std::vector<FlagArgument> _flagArgs;
    std::map<std::string, size_t> _flagArgsNameTable;
    std::map<char, size_t> _flagArgsShortNameTable;

    std::vector<ValueArgument> _valueArgs;
    std::map<std::string, size_t> _valueArgsNameTable;
    std::map<char, size_t> _valueArgsShortNameTable;

    uint32_t _minPositionalArgNum = 0;
    uint32_t _maxPositionalArgNum = 0;
    std::optional<std::string> _positionalArgDescription;

    std::map<std::string, Parser> _subParsers;

    /// If the current command is a sub command, need to record the names of the
    /// parent commands here in hierarchical order.
    std::vector<std::string> _preCommands;

  private:

    /// Private constructor.
    /// Use `constructFrom` function to create a parser.
    Parser() = default;

  private:

    /// Construct a parser from the command group.
    ///
    /// @param[in] preCommands If the current command group is a sub command,
    /// you need to specify the names of the parent commands in hierarchical
    /// order.
    /// @return Return `true` if the command group config is valid.
    bool _constructFrom(
        const CommandGroup& cg,
        const std::vector<std::string>& preCommands = {},
        const Logger& logger = Logger()
    );

    /// Parse command line arguments into ValueTree.
    /// Return `true` if the input arguments are valid.
    bool _parse(
        ValueTree& tree,
        int argc,
        const char* const argv[],
        const Logger& logger = Logger()
    ) const;

    /// Generate help message of specified command group.
    ///
    /// @return Return std::nullopt if the specified command group does not
    /// exist.
    std::optional<std::string> _getHelp(
        bool enableAnsiFormat = true, const Logger& logger = Logger()
    ) const;
};

}  // namespace cli
}  // namespace c2p

#endif  // __C2P_CLI_HPP__
