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
    uint32_t MinPositionalArgNum = 0;

    /// Maximum allowed number of positional arguments.
    uint32_t MaxPositionalArgNum = 0;

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
    /// Should be called after `construct()`, and `isValid()` must return true.
    /// If the input arguments are invalid, return an empty ValueTree.
    ValueTree
    parse(int argc, const char* const argv[], const Logger& logger = Logger());

    /// Generate help message of specified command group.
    ///
    /// @param[in] subCommands If you want to get the help information of a sub
    /// command, you need to specify the names in hierarchical order.
    /// @return Return std::nullopt if the specified command group does not
    /// exist.
    std::optional<std::string> getHelp(
        const std::vector<std::string>& subCommands = {},
        const Logger& logger = Logger()
    ) const;

  private:

    std::string _command;
    std::optional<std::string> _description;

    uint32_t _MinPositionalArgNum = 0;
    uint32_t _MaxPositionalArgNum = 0;

    std::map<std::string, Parser> _subParsers;

  public:

    Parser(const Parser&) = delete;
    Parser(Parser&&) = default;
    Parser& operator=(const Parser&) = delete;
    Parser& operator=(Parser&&) = default;
    ~Parser() = default;

  private:

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

    /// Generate help message of specified command group.
    ///
    /// @param[in] subCommands If you want to get the help information of a sub
    /// command, you need to specify the names in hierarchical order.
    /// @param[in] preCommands If the current command group is a sub command,
    /// you need to specify the names of the parent commands in hierarchical
    /// order.
    /// @return Return std::nullopt if the specified command group does not
    /// exist.
    std::optional<std::string> _getHelp(
        const std::vector<std::string>& subCommands = {},
        const std::vector<std::string>& preCommands = {},
        const Logger& logger = Logger()
    ) const;
};

}  // namespace cli
}  // namespace c2p

#endif  // __C2P_CLI_HPP__
