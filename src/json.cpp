/// Extended JSON PEG Grammar:
///
/// ```peg
/// JSON       <- S Value S
///
/// Value      <- Object
///            / Array
///            / String
///            / Number
///            / 'true'
///            / 'false'
///            / 'null'
///
/// Object     <- '{' S Members? S '}'
/// Members    <- Member (S ',' S Member)* (S ',')?  // trailing comma is OK
/// Member     <- String S ':' S Value
///
/// Array      <- '[' S Elements? S ']'
/// Elements   <- Value (S ',' S Value)* (S ',')?  // trailing comma is OK
///
/// String     <- '"' Characters '"'
/// Characters <- Character*
/// Character  <- !["\\] .
///            / '\\' Escape
/// Escape     <- ["\\/bfnrt]
///            / 'u' Hex Hex Hex Hex
/// Hex        <- [0-9a-fA-F]
///
/// Number     <- ('+' / '-')? Integer Fraction? Exponent?  // '+' is OK
/// Integer    <- '0' / [1-9] [0-9]*
/// Fraction   <- '.' [0-9]+
/// Exponent   <- [eE] [+-]? [0-9]+
///
/// S          <- (Whitespace / Comment)*
/// Whitespace <- [ \t\n\r]
/// Comment    <- '//' (![\n\r] .)*  // single-line comment starts with '//'
/// ```

#include "c2p/json.hpp"

#include <sstream>

namespace c2p {
namespace json {

static bool _parseWhitespace(const std::string& json, size_t& pos) {
    if (pos >= json.size() || !std::isspace(uint8_t(json[pos]))) return false;
    ++pos;
    return true;
}

static bool _parseComment(const std::string& json, size_t& pos) {
    if (pos + 1 >= json.size() || json[pos] != '/' || json[pos + 1] != '/') {
        return false;
    }
    pos += 2;
    while (pos < json.size() && json[pos] != '\n' && json[pos] != '\r') ++pos;
    return true;
}

static void _skipWhitespace(const std::string& json, size_t& pos) {
    while (_parseWhitespace(json, pos) || _parseComment(json, pos));
}

static bool _parseString(
    ValueTree& tree, const std::string& json, size_t& pos, const Logger& logger
) {
    ++pos;  // Skip initial quote
    std::string result;
    while (pos < json.size() && json[pos] != '"') {
        if (json[pos] == '\\') {
            ++pos;
            if (pos >= json.size()) {
                logger.logError("Unexpected end of input in string escape.");
                return false;
            }
            switch (json[pos]) {
                case '"': result.push_back('"'); break;
                case '\\': result.push_back('\\'); break;
                case '/': result.push_back('/'); break;
                case 'b': result.push_back('\b'); break;
                case 'f': result.push_back('\f'); break;
                case 'n': result.push_back('\n'); break;
                case 'r': result.push_back('\r'); break;
                case 't': result.push_back('\t'); break;
                case 'u': {
                    if (pos + 4 >= json.size()) {
                        logger.logError("Invalid Unicode escape sequence.");
                        return false;
                    }
                    std::string hex = json.substr(pos + 1, 4);
                    result.push_back(
                        static_cast<char>(std::stoi(hex, nullptr, 16))
                    );
                    pos += 4;
                    break;
                }
                default: {
                    logger.logError("Invalid escape character.");
                    return false;
                }
            }
        } else {
            result.push_back(json[pos]);
        }
        ++pos;
    }
    if (pos >= json.size() || json[pos] != '"') {
        logger.logError("Unterminated string.");
        return false;
    }
    ++pos;  // Skip closing quote
    tree = result;
    return true;
}

static bool _parseValue(
    ValueTree& tree, const std::string& json, size_t& pos, const Logger& logger
);

static bool _parseObject(
    ValueTree& tree, const std::string& json, size_t& pos, const Logger& logger
) {
    auto& object = tree.asObject();
    ++pos;  // Skip initial brace
    _skipWhitespace(json, pos);
    if (pos < json.size() && json[pos] == '}') {
        // Empty object
        ++pos;
        return true;
    }
    while (pos < json.size()) {
        _skipWhitespace(json, pos);
        ValueTree key;
        if (!_parseString(key, json, pos, logger)) {
            logger.logError("Failed to parse object key.");
            return false;
        }
        _skipWhitespace(json, pos);
        if (pos >= json.size() || json[pos] != ':') {
            logger.logError("Expected ':' in object.");
            return false;
        }
        ++pos;
        _skipWhitespace(json, pos);
        ValueTree& value = object[*key.value<TypeTag::STRING>()];
        if (!_parseValue(value, json, pos, logger)) {
            logger.logError("Failed to parse object value.");
            return false;
        }
        _skipWhitespace(json, pos);
        if (pos < json.size() && json[pos] == '}') {
            ++pos;
            return true;
        }
        if (pos >= json.size() || json[pos] != ',') {
            logger.logError("Expected ',' or '}' in object.");
            return false;
        }
        ++pos;
        _skipWhitespace(json, pos);
        if (pos < json.size() && json[pos] == '}') {
            ++pos;
            return true;
        }
    }
    logger.logError("Unterminated object.");
    return false;
}

static bool _parseArray(
    ValueTree& tree, const std::string& json, size_t& pos, const Logger& logger
) {
    auto& array = tree.asArray();
    ++pos;  // Skip initial bracket
    _skipWhitespace(json, pos);
    if (pos < json.size() && json[pos] == ']') {
        ++pos;
        return true;
    }
    while (pos < json.size()) {
        _skipWhitespace(json, pos);
        array.push_back(ValueTree());
        if (!_parseValue(array.back(), json, pos, logger)) {
            logger.logError("Failed to parse array value.");
            return false;
        }
        _skipWhitespace(json, pos);
        if (pos < json.size() && json[pos] == ']') {
            ++pos;
            return true;
        }
        if (pos >= json.size() || json[pos] != ',') {
            logger.logError("Expected ',' or ']' in array.");
            return false;
        }
        ++pos;
        _skipWhitespace(json, pos);
        if (pos < json.size() && json[pos] == ']') {
            ++pos;
            return true;
        }
    }
    logger.logError("Unterminated array.");
    return false;
}

static bool _parseNumber(
    ValueTree& tree, const std::string& json, size_t& pos, const Logger& logger
) {
    size_t start = pos;
    if (json[pos] == '+' || json[pos] == '-') ++pos;
    if (pos >= json.size()) {
        logger.logError("Invalid number.");
        return false;
    }
    if (json[pos] == '0') {
        ++pos;
    } else {
        if (!std::isdigit(json[pos])) {
            logger.logError("Invalid number.");
            return false;
        }
        while (pos < json.size() && std::isdigit(json[pos])) ++pos;
    }
    if (pos < json.size() && json[pos] == '.') {
        ++pos;
        if (pos >= json.size() || !std::isdigit(json[pos])) {
            logger.logError("Invalid number.");
            return false;
        }
        while (pos < json.size() && std::isdigit(json[pos])) ++pos;
    }
    if (pos < json.size() && (json[pos] == 'e' || json[pos] == 'E')) {
        ++pos;
        if (pos < json.size() && (json[pos] == '+' || json[pos] == '-')) ++pos;
        if (pos >= json.size() || !std::isdigit(json[pos])) {
            logger.logError("Invalid number.");
            return false;
        }
        while (pos < json.size() && std::isdigit(json[pos])) ++pos;
    }
    tree = std::stod(json.substr(start, pos - start));
    return true;
}

static bool _parseTrue(
    ValueTree& tree, const std::string& json, size_t& pos, const Logger& logger
) {
    if (json.substr(pos, 4) != "true") {
        logger.logError("Invalid value. Expected to be \"true\".");
        return false;
    }
    pos += 4;
    tree = true;
    return true;
}

static bool _parseFalse(
    ValueTree& tree, const std::string& json, size_t& pos, const Logger& logger
) {
    if (json.substr(pos, 5) != "false") {
        logger.logError("Invalid value. Expected to be \"false\".");
        return false;
    }
    pos += 5;
    tree = false;
    return true;
}

static bool _parseNull(
    ValueTree& tree, const std::string& json, size_t& pos, const Logger& logger
) {
    if (json.substr(pos, 4) != "null") {
        logger.logError("Invalid value. Expected to be \"null\".");
        return false;
    }
    pos += 4;
    tree = NONE;
    return true;
}

static bool _parseValue(
    ValueTree& tree, const std::string& json, size_t& pos, const Logger& logger
) {
    if (json[pos] == '{') return _parseObject(tree, json, pos, logger);
    if (json[pos] == '[') return _parseArray(tree, json, pos, logger);
    if (json[pos] == '"') return _parseString(tree, json, pos, logger);
    if (json[pos] == 't') return _parseTrue(tree, json, pos, logger);
    if (json[pos] == 'f') return _parseFalse(tree, json, pos, logger);
    if (json[pos] == 'n') return _parseNull(tree, json, pos, logger);
    if (json[pos] == '+' || json[pos] == '-' || std::isdigit(json[pos]))
        return _parseNumber(tree, json, pos, logger);
    logger.logError(
        std::string("Invalid JSON value with head: '") + json[pos] + "'."
    );
    return false;
}

ValueTree parse(const std::string& json, const Logger& logger) {
    ValueTree tree;
    size_t pos = 0;
    _skipWhitespace(json, pos);
    if (!_parseValue(tree, json, pos, logger)) {
        logger.logError("Failed to parse JSON.");
        return ValueTree();
    }
    _skipWhitespace(json, pos);
    if (pos != json.size()) {
        logger.logWarning("Extra characters after JSON.");
    }
    return tree;
}

static void _escapeString(const std::string& input, std::stringstream& stream) {
    for (char c: input) {
        switch (c) {
            case '"': stream << "\\\""; break;
            case '\\': stream << "\\\\"; break;
            case '\b': stream << "\\b"; break;
            case '\f': stream << "\\f"; break;
            case '\n': stream << "\\n"; break;
            case '\r': stream << "\\r"; break;
            case '\t': stream << "\\t"; break;
            default: stream << c; break;
        }
    }
}

static void _dump(
    ValueTree& tree,
    std::stringstream& stream,
    bool pretty,
    size_t indent,
    size_t indentStep
) {
    size_t newIndent = indent + indentStep;
    switch (tree.state()) {
        case ValueTree::State::EMPTY: break;

        case ValueTree::State::VALUE: {
            const auto& node = tree.asValue();
            switch (node.typeTag()) {
                case TypeTag::NONE: {
                    stream << "null";
                } break;

                case TypeTag::BOOL: {
                    stream << (*node.value<TypeTag::BOOL>() ? "true" : "false");
                } break;

                case TypeTag::NUMBER: {
                    stream << *node.value<TypeTag::NUMBER>();
                } break;

                case TypeTag::STRING: {
                    stream << '"';
                    _escapeString(*node.value<TypeTag::STRING>(), stream);
                    stream << '"';
                } break;
            }
        } break;

        case ValueTree::State::ARRAY: {
            auto& array = tree.asArray();
            if (array.empty()) {
                stream << "[]";
                break;
            }
            if (pretty) {
                stream << '[';
                bool first = true;
                for (auto& value: array) {
                    if (first) first = false;
                    else stream << ',';
                    stream << '\n' << std::string(newIndent, ' ');
                    _dump(value, stream, pretty, newIndent, indentStep);
                }
                stream << '\n' << std::string(indent, ' ') << ']';
            } else {
                stream << '[';
                bool first = true;
                for (auto& value: array) {
                    if (first) first = false;
                    else stream << ',';
                    _dump(value, stream, pretty, newIndent, indentStep);
                }
                stream << ']';
            }
        } break;

        case ValueTree::State::OBJECT: {
            auto& object = tree.asObject();
            if (object.empty()) {
                stream << "{}";
                break;
            }
            if (pretty) {
                stream << '{';
                bool first = true;
                for (auto& [key, value]: object) {
                    if (first) first = false;
                    else stream << ',';
                    stream << '\n'
                           << std::string(newIndent, ' ') << '"' << key << '"'
                           << ": ";
                    _dump(value, stream, pretty, newIndent, indentStep);
                }
                stream << '\n' << std::string(indent, ' ') << '}';
            } else {
                stream << '{';
                bool first = true;
                for (auto& [key, value]: object) {
                    if (first) first = false;
                    else stream << ',';
                    stream << '"' << key << '"' << ':';
                    _dump(value, stream, pretty, newIndent, indentStep);
                }
                stream << '}';
            }
        } break;
    }
}

std::string dump(ValueTree& tree, bool pretty, size_t indentStep) {
    std::stringstream stream;
    _dump(tree, stream, pretty, 0, indentStep);
    return stream.str();
}

}  // namespace json
}  // namespace c2p
