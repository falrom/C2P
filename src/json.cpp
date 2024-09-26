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
#include "text_utils.hpp"

#include <sstream>

namespace c2p {
namespace json {

static bool _parseWhitespace(const TextContext& ctx, TextPosition& pos) {
    if (!pos.valid || !std::isspace(uint8_t(ctx.text[pos.pos]))) return false;
    ctx.moveForward(pos);
    return true;
}

static bool _parseComment(const TextContext& ctx, TextPosition& pos) {
    auto aheadPos = pos;
    if (!ctx.moveForward(aheadPos)   //
        || ctx.text[pos.pos] != '/'  //
        || ctx.text[aheadPos.pos] != '/')
    {
        return false;
    }
    ctx.moveNextLine(pos);
    return true;
}

static void _skipWhitespace(const TextContext& ctx, TextPosition& pos) {
    while (_parseWhitespace(ctx, pos) || _parseComment(ctx, pos));
}

static bool _parseString(
    ValueTree& tree,
    const TextContext& ctx,
    TextPosition& pos,
    const Logger& logger
) {
    ctx.moveForward(pos);  // Skip initial quote
    std::string result;
    while (pos.valid && ctx.text[pos.pos] != '"') {
        if (ctx.text[pos.pos] == '\\') {
            if (!ctx.moveForward(pos)) {
                logger.logError(
                    pos.toString()
                    + ": Unexpected end of input in string escape."
                );
                return false;
            }
            switch (ctx.text[pos.pos]) {
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
                    if (!ctx.moveForward(aheadPos, 4)) {
                        logger.logError(
                            pos.toString()
                            + ": Invalid Unicode escape sequence."
                        );
                        return false;
                    }
                    ctx.moveForward(pos);
                    result.push_back(static_cast<char>(
                        std::stoi(std::string(ctx.slice(pos, 4)), nullptr, 16)
                    ));
                    ctx.moveForward(aheadPos);
                    pos = aheadPos;
                    break;
                }
                default: {
                    logger.logError(
                        pos.toString() + ": Invalid escape character."
                    );
                    return false;
                }
            }
        } else {
            result.push_back(ctx.text[pos.pos]);
        }
        ctx.moveForward(pos);
    }
    if (!pos.valid || ctx.text[pos.pos] != '"') {
        logger.logError(pos.toString() + ": Unterminated string.");
        return false;
    }
    ctx.moveForward(pos);  // Skip closing quote
    tree = result;
    return true;
}

static bool _parseValue(
    ValueTree& tree,
    const TextContext& ctx,
    TextPosition& pos,
    const Logger& logger
);

static bool _parseObject(
    ValueTree& tree,
    const TextContext& ctx,
    TextPosition& pos,
    const Logger& logger
) {
    auto& object = tree.asObject();
    ctx.moveForward(pos);  // Skip initial brace
    _skipWhitespace(ctx, pos);
    if (pos.valid && ctx.text[pos.pos] == '}') {
        // Empty object
        ctx.moveForward(pos);
        return true;
    }
    while (pos.valid) {
        _skipWhitespace(ctx, pos);
        ValueTree key;
        if (!_parseString(key, ctx, pos, logger)) {
            logger.logError(pos.toString() + ": Failed to parse object key.");
            return false;
        }
        _skipWhitespace(ctx, pos);
        if (!pos.valid || ctx.text[pos.pos] != ':') {
            logger.logError(pos.toString() + ": Expected ':' in object.");
            return false;
        }
        ctx.moveForward(pos);
        _skipWhitespace(ctx, pos);
        ValueTree& value = object[*key.value<TypeTag::STRING>()];
        if (!_parseValue(value, ctx, pos, logger)) {
            logger.logError(pos.toString() + ": Failed to parse object value.");
            return false;
        }
        _skipWhitespace(ctx, pos);
        if (pos.valid && ctx.text[pos.pos] == '}') {
            ctx.moveForward(pos);
            return true;
        }
        if (!pos.valid || ctx.text[pos.pos] != ',') {
            logger.logError(
                pos.toString() + ": Expected ',' or '}' in object."
            );
            return false;
        }
        ctx.moveForward(pos);
        _skipWhitespace(ctx, pos);
        if (pos.valid && ctx.text[pos.pos] == '}') {
            ctx.moveForward(pos);
            return true;
        }
    }
    logger.logError(pos.toString() + ": Unterminated object.");
    return false;
}

static bool _parseArray(
    ValueTree& tree,
    const TextContext& ctx,
    TextPosition& pos,
    const Logger& logger
) {
    auto& array = tree.asArray();
    ctx.moveForward(pos);  // Skip initial bracket
    _skipWhitespace(ctx, pos);
    if (pos.valid && ctx.text[pos.pos] == ']') {
        ctx.moveForward(pos);
        return true;
    }
    while (pos.valid) {
        _skipWhitespace(ctx, pos);
        array.push_back(ValueTree());
        if (!_parseValue(array.back(), ctx, pos, logger)) {
            logger.logError(pos.toString() + ": Failed to parse array value.");
            return false;
        }
        _skipWhitespace(ctx, pos);
        if (pos.valid && ctx.text[pos.pos] == ']') {
            ctx.moveForward(pos);
            return true;
        }
        if (!pos.valid || ctx.text[pos.pos] != ',') {
            logger.logError(pos.toString() + ": Expected ',' or ']' in array.");
            return false;
        }
        ctx.moveForward(pos);
        _skipWhitespace(ctx, pos);
        if (pos.valid && ctx.text[pos.pos] == ']') {
            ctx.moveForward(pos);
            return true;
        }
    }
    logger.logError(pos.toString() + ": Unterminated array.");
    return false;
}

static bool _parseNumber(
    ValueTree& tree,
    const TextContext& ctx,
    TextPosition& pos,
    const Logger& logger
) {
    auto start = pos;
    if (ctx.text[pos.pos] == '+' || ctx.text[pos.pos] == '-') {
        ctx.moveForward(pos);
    }
    if (!pos.valid) {
        logger.logError(pos.toString() + ": Invalid number.");
        return false;
    }
    if (ctx.text[pos.pos] == '0') {
        ctx.moveForward(pos);
    } else {
        if (!std::isdigit(ctx.text[pos.pos])) {
            logger.logError(pos.toString() + ": Invalid number.");
            return false;
        }
        while (pos.valid && std::isdigit(ctx.text[pos.pos])) {
            ctx.moveForward(pos);
        }
    }
    if (pos.valid && ctx.text[pos.pos] == '.') {
        ctx.moveForward(pos);
        if (!pos.valid || !std::isdigit(ctx.text[pos.pos])) {
            logger.logError(pos.toString() + ": Invalid number.");
            return false;
        }
        while (pos.valid && std::isdigit(ctx.text[pos.pos]))
            ctx.moveForward(pos);
    }
    if (pos.valid && (ctx.text[pos.pos] == 'e' || ctx.text[pos.pos] == 'E')) {
        if (ctx.moveForward(pos)
            && (ctx.text[pos.pos] == '+' || ctx.text[pos.pos] == '-'))
        {
            ctx.moveForward(pos);
        }
        if (!pos.valid || !std::isdigit(ctx.text[pos.pos])) {
            logger.logError(pos.toString() + ": Invalid number.");
            return false;
        }
        while (pos.valid && std::isdigit(ctx.text[pos.pos])) {
            ctx.moveForward(pos);
        }
    }
    tree = std::stod(std::string(ctx.slice(start, pos)));
    return true;
}

static bool _parseTrue(
    ValueTree& tree,
    const TextContext& ctx,
    TextPosition& pos,
    const Logger& logger
) {
    if (ctx.slice(pos, 4) != "true") {
        logger.logError(
            pos.toString() + ": Invalid value. Expected to be \"true\"."
        );
        return false;
    }
    ctx.moveForward(pos, 4);
    tree = true;
    return true;
}

static bool _parseFalse(
    ValueTree& tree,
    const TextContext& ctx,
    TextPosition& pos,
    const Logger& logger
) {
    if (ctx.slice(pos, 5) != "false") {
        logger.logError(
            pos.toString() + ": Invalid value. Expected to be \"false\"."
        );
        return false;
    }
    ctx.moveForward(pos, 5);
    tree = false;
    return true;
}

static bool _parseNull(
    ValueTree& tree,
    const TextContext& ctx,
    TextPosition& pos,
    const Logger& logger
) {
    if (ctx.slice(pos, 4) != "null") {
        logger.logError(
            pos.toString() + ": Invalid value. Expected to be \"null\"."
        );
        return false;
    }
    ctx.moveForward(pos, 4);
    tree = NONE;
    return true;
}

static bool _parseValue(
    ValueTree& tree,
    const TextContext& ctx,
    TextPosition& pos,
    const Logger& logger
) {
    const auto ch = ctx.text[pos.pos];
    if (ch == '{') return _parseObject(tree, ctx, pos, logger);
    if (ch == '[') return _parseArray(tree, ctx, pos, logger);
    if (ch == '"') return _parseString(tree, ctx, pos, logger);
    if (ch == 't') return _parseTrue(tree, ctx, pos, logger);
    if (ch == 'f') return _parseFalse(tree, ctx, pos, logger);
    if (ch == 'n') return _parseNull(tree, ctx, pos, logger);
    if (ch == '+' || ch == '-' || std::isdigit(ch))
        return _parseNumber(tree, ctx, pos, logger);
    logger.logError(
        pos.toString() + ": Invalid JSON value with head: '" + ch + "'."
    );
    return false;
}

ValueTree parse(const std::string& json, const Logger& logger) {
    if (json.empty()) {
        logger.logError("Empty JSON.");
        return ValueTree();
    }

    TextContext ctx = { json };
    TextPosition pos = { .valid = true, .pos = 0, .lineIdx = 0, .linePos = 0 };

    ValueTree tree;
    _skipWhitespace(ctx, pos);
    if (!_parseValue(tree, ctx, pos, logger)) {
        logger.logError("Failed to parse JSON.");
        return ValueTree();
    }
    _skipWhitespace(ctx, pos);

    if (pos.valid) {
        logger.logWarning(pos.toString() + ": Extra characters after JSON.");
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
