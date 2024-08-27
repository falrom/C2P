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

using namespace c2p::value_tree;

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
    ValueNode& node, const std::string& json, size_t& pos, const Logger& logger
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
    node = result;
    return true;
}

static bool _parseValue(
    ValueNode& node, const std::string& json, size_t& pos, const Logger& logger
);

static bool _parseObject(
    ValueNode& node, const std::string& json, size_t& pos, const Logger& logger
) {
    node = ObjectValue();
    ++pos;  // Skip initial brace
    _skipWhitespace(json, pos);
    if (pos < json.size() && json[pos] == '}') {
        // Empty object
        ++pos;
        return true;
    }
    auto& object = node.asObject();
    while (pos < json.size()) {
        _skipWhitespace(json, pos);
        ValueNode key;
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
        ValueNode& value = object[*key.value<ValueType::STRING>()];
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
    ValueNode& node, const std::string& json, size_t& pos, const Logger& logger
) {
    node = ArrayValue();
    ++pos;  // Skip initial bracket
    _skipWhitespace(json, pos);
    if (pos < json.size() && json[pos] == ']') {
        ++pos;
        return true;
    }
    auto& array = node.asArray();
    while (pos < json.size()) {
        _skipWhitespace(json, pos);
        array.push_back(ValueNode());
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
    ValueNode& node, const std::string& json, size_t& pos, const Logger& logger
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
    node = std::stod(json.substr(start, pos - start));
    return true;
}

static bool _parseTrue(
    ValueNode& node, const std::string& json, size_t& pos, const Logger& logger
) {
    if (json.substr(pos, 4) != "true") {
        logger.logError("Invalid value. Expected to be \"true\".");
        return false;
    }
    pos += 4;
    node = true;
    return true;
}

static bool _parseFalse(
    ValueNode& node, const std::string& json, size_t& pos, const Logger& logger
) {
    if (json.substr(pos, 5) != "false") {
        logger.logError("Invalid value. Expected to be \"false\".");
        return false;
    }
    pos += 5;
    node = false;
    return true;
}

static bool _parseNull(
    ValueNode& node, const std::string& json, size_t& pos, const Logger& logger
) {
    if (json.substr(pos, 4) != "null") {
        logger.logError("Invalid value. Expected to be \"null\".");
        return false;
    }
    pos += 4;
    node = NONE;
    return true;
}

static bool _parseValue(
    ValueNode& node, const std::string& json, size_t& pos, const Logger& logger
) {
    if (json[pos] == '{') return _parseObject(node, json, pos, logger);
    if (json[pos] == '[') return _parseArray(node, json, pos, logger);
    if (json[pos] == '"') return _parseString(node, json, pos, logger);
    if (json[pos] == 't') return _parseTrue(node, json, pos, logger);
    if (json[pos] == 'f') return _parseFalse(node, json, pos, logger);
    if (json[pos] == 'n') return _parseNull(node, json, pos, logger);
    if (json[pos] == '+' || json[pos] == '-' || std::isdigit(json[pos]))
        return _parseNumber(node, json, pos, logger);
    logger.logError(
        std::string("Invalid JSON value with head: '") + json[pos] + "'."
    );
    return false;
}

ValueNode parse(const std::string& json, const Logger& logger) {
    ValueNode node;
    size_t pos = 0;
    _skipWhitespace(json, pos);
    if (!_parseValue(node, json, pos, logger)) {
        logger.logError("Failed to parse JSON.");
        return NONE;
    }
    _skipWhitespace(json, pos);
    if (pos != json.size()) {
        logger.logWarning("Extra characters after JSON.");
    }
    return node;
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
    ValueNode& node,
    std::stringstream& stream,
    bool pretty,
    size_t indent,
    size_t indentStep
) {
    size_t newIndent = indent + indentStep;
    switch (node.type()) {
        case ValueType::NONE: {
            stream << "null";
        } break;

        case ValueType::BOOL: {
            stream << (*node.value<ValueType::BOOL>() ? "true" : "false");
        } break;

        case ValueType::NUMBER: {
            stream << *node.value<ValueType::NUMBER>();
        } break;

        case ValueType::STRING: {
            stream << '"';
            _escapeString(*node.value<ValueType::STRING>(), stream);
            stream << '"';
        } break;

        case ValueType::ARRAY: {
            auto& array = node.asArray();
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

        case ValueType::OBJECT: {
            auto& object = node.asObject();
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

std::string dump(ValueNode& node, bool pretty, size_t indentStep) {
    if (node.type() == ValueType::NONE) return "";
    std::stringstream stream;
    _dump(node, stream, pretty, 0, indentStep);
    return stream.str();
}

}  // namespace json
}  // namespace c2p
