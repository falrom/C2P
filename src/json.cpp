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

#include <cassert>
#include <sstream>

namespace c2p {
namespace json {

static void _logErrorAtPos(
    const Logger& logger,
    const TextContext& ctx,
    const PositionInText& pos,
    const std::string& msg
) {
    logger.error(
        pos.toString() + ": " + msg +
        [](const std::vector<std::string>& msgLines) {
            std::string msg;
            for (const auto& msgLine: msgLines) {
                msg += "\n" + msgLine;
            }
            msg += "\n";
            return msg;
        }(getPositionMessage(ctx, pos))
    );
}

static bool _parseWhitespace(const TextContext& ctx, PositionInText& pos) {
    if (!pos.valid || !std::isspace(uint8_t(ctx.text[pos.pos]))) {
        return false;
    }
    ctx.moveForward(pos);
    return true;
}

static bool _parseComment(const TextContext& ctx, PositionInText& pos) {
    auto aheadPos = pos;
    if (!ctx.moveForward(aheadPos)   //
        || ctx.text[pos.pos] != '/'  //
        || ctx.text[aheadPos.pos] != '/')
    {
        return false;
    }
    ctx.moveToNextLine(pos);
    return true;
}

static void _skipWhitespace(const TextContext& ctx, PositionInText& pos) {
    while (_parseWhitespace(ctx, pos) || _parseComment(ctx, pos));
}

static bool _parseString(
    ValueTree& tree,
    const TextContext& ctx,
    PositionInText& pos,
    const Logger& logger
) {
    assert(pos.valid);
    assert(ctx.text[pos.pos] == '"');

    const auto leftQuotePos = pos;

    // Skip initial quote
    if (!ctx.moveForwardInLine(pos)) {
        _logErrorAtPos(
            logger,
            ctx,
            leftQuotePos,
            "Unterminated quoted string. "
            "Expected closing quote '\"' in same line."
        );
        return false;
    }

    std::string result;
    while (ctx.text[pos.pos] != '"' && !ctx.atLineEnd(pos)) {
        if (ctx.text[pos.pos] == '\\') {
            if (!ctx.moveForwardInLine(pos)) {
                _logErrorAtPos(
                    logger,
                    ctx,
                    pos,
                    "Unexpected end of input in string escape."
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
                    for (int idx = 0; idx < 4; ++idx) {
                        if (!ctx.moveForwardInLine(aheadPos)) {
                            _logErrorAtPos(
                                logger,
                                ctx,
                                pos,
                                "Unexpected end of input in Unicode escape. "
                                "Need 4 Hex digits like: \"\\uHHHH\"."
                            );
                            return false;
                        }
                        if (!std::isxdigit(ctx.text[aheadPos.pos])) {
                            _logErrorAtPos(
                                logger,
                                ctx,
                                aheadPos,
                                "Invalid Unicode escape character. "
                                "Need 4 Hex digits like: \"\\uHHHH\"."
                            );
                            return false;
                        }
                    }
                    ctx.moveForwardInLine(pos);
                    result += unicodeToUtf8(uint32_t(
                        std::stoul(std::string(ctx.slice(pos, 4)), nullptr, 16)
                    ));
                    pos = aheadPos;
                    break;
                }
                case 'U': {
                    auto aheadPos = pos;
                    for (int idx = 0; idx < 8; ++idx) {
                        if (!ctx.moveForwardInLine(aheadPos)) {
                            _logErrorAtPos(
                                logger,
                                ctx,
                                pos,
                                "Unexpected end of input in Unicode escape. "
                                "Need 8 Hex digits like: \"\\UHHHHHHHH\"."
                            );
                            return false;
                        }
                        if (!std::isxdigit(ctx.text[aheadPos.pos])) {
                            _logErrorAtPos(
                                logger,
                                ctx,
                                aheadPos,
                                "Invalid Unicode escape character. "
                                "Need 8 Hex digits like: \"\\UHHHHHHHH\"."
                            );
                            return false;
                        }
                    }
                    ctx.moveForwardInLine(pos);
                    result += unicodeToUtf8(uint32_t(
                        std::stoul(std::string(ctx.slice(pos, 8)), nullptr, 16)
                    ));
                    pos = aheadPos;
                    break;
                }
                default: {
                    _logErrorAtPos(
                        logger, ctx, pos, "Invalid escape character."
                    );
                    return false;
                }
            }
        } else {
            result.push_back(ctx.text[pos.pos]);
        }
        ctx.moveForwardInLine(pos);
    }
    if (ctx.text[pos.pos] != '"') {
        _logErrorAtPos(
            logger,
            ctx,
            pos,
            "Unterminated string. "
            "Expected closing quote '\"' in same line."
        );
        return false;
    }
    ctx.moveForward(pos);  // Skip closing quote

    tree = result;
    return true;
}

static bool _parseValue(
    ValueTree& tree,
    const TextContext& ctx,
    PositionInText& pos,
    const Logger& logger
);

static bool _parseObject(
    ValueTree& tree,
    const TextContext& ctx,
    PositionInText& pos,
    const Logger& logger
) {
    assert(pos.valid);
    assert(ctx.text[pos.pos] == '{');
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
        if (ctx.text[pos.pos] != '"') {
            _logErrorAtPos(
                logger,
                ctx,
                pos,
                "Expected quoted string with '\"' as object key."
            );
            return false;
        }
        const auto keyStartPos = pos;
        ValueTree key;
        if (!_parseString(key, ctx, pos, logger)) {
            _logErrorAtPos(
                logger, ctx, keyStartPos, "Failed to parse object key."
            );
            return false;
        }
        _skipWhitespace(ctx, pos);
        if (!pos.valid || ctx.text[pos.pos] != ':') {
            _logErrorAtPos(logger, ctx, pos, "Expected ':' in object.");
            return false;
        }
        ctx.moveForward(pos);
        _skipWhitespace(ctx, pos);
        const auto valueStartPos = pos;
        ValueTree& value = object[*key.value<TypeTag::STRING>()];
        if (!_parseValue(value, ctx, pos, logger)) {
            _logErrorAtPos(
                logger, ctx, valueStartPos, "Failed to parse object value."
            );
            return false;
        }
        const auto afterValuePos = pos;
        _skipWhitespace(ctx, pos);
        if (pos.valid && ctx.text[pos.pos] == '}') {
            ctx.moveForward(pos);
            return true;
        }
        if (!pos.valid || ctx.text[pos.pos] != ',') {
            _logErrorAtPos(
                logger,
                ctx,
                afterValuePos,
                "Expected ',' or '}' at the end of object."
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
    _logErrorAtPos(logger, ctx, pos, "Unterminated object.");
    return false;
}

static bool _parseArray(
    ValueTree& tree,
    const TextContext& ctx,
    PositionInText& pos,
    const Logger& logger
) {
    assert(pos.valid);
    assert(ctx.text[pos.pos] == '[');
    auto& array = tree.asArray();
    ctx.moveForward(pos);  // Skip initial bracket
    _skipWhitespace(ctx, pos);
    if (pos.valid && ctx.text[pos.pos] == ']') {
        ctx.moveForward(pos);
        return true;
    }
    while (pos.valid) {
        _skipWhitespace(ctx, pos);
        const auto valueStartPos = pos;
        array.push_back(ValueTree());
        if (!_parseValue(array.back(), ctx, pos, logger)) {
            _logErrorAtPos(
                logger, ctx, valueStartPos, "Failed to parse array value."
            );
            return false;
        }
        _skipWhitespace(ctx, pos);
        if (pos.valid && ctx.text[pos.pos] == ']') {
            ctx.moveForward(pos);
            return true;
        }
        if (!pos.valid || ctx.text[pos.pos] != ',') {
            _logErrorAtPos(logger, ctx, pos, "Expected ',' or ']' in array.");
            return false;
        }
        ctx.moveForward(pos);
        _skipWhitespace(ctx, pos);
        if (pos.valid && ctx.text[pos.pos] == ']') {
            ctx.moveForward(pos);
            return true;
        }
    }
    _logErrorAtPos(logger, ctx, pos, "Unterminated array.");
    return false;
}

static bool _parseNumber(
    ValueTree& tree,
    const TextContext& ctx,
    PositionInText& pos,
    const Logger& logger
) {
    assert(pos.valid);
    const auto startPos = pos;
    if (ctx.text[pos.pos] == '+' || ctx.text[pos.pos] == '-') {
        ctx.moveForward(pos);
    }
    if (!pos.valid) {
        _logErrorAtPos(logger, ctx, pos, "Invalid number.");
        return false;
    }
    if (ctx.text[pos.pos] == '0') {
        ctx.moveForward(pos);
    } else {
        if (!std::isdigit(ctx.text[pos.pos])) {
            _logErrorAtPos(logger, ctx, pos, "Invalid number.");
            return false;
        }
        while (pos.valid && std::isdigit(ctx.text[pos.pos])) {
            ctx.moveForward(pos);
        }
    }
    if (pos.valid && ctx.text[pos.pos] == '.') {
        ctx.moveForward(pos);
        if (!pos.valid || !std::isdigit(ctx.text[pos.pos])) {
            _logErrorAtPos(logger, ctx, pos, "Invalid number.");
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
            _logErrorAtPos(logger, ctx, pos, "Invalid number.");
            return false;
        }
        while (pos.valid && std::isdigit(ctx.text[pos.pos])) {
            ctx.moveForward(pos);
        }
    }
    tree = std::stod(std::string(ctx.slice(startPos, pos)));
    return true;
}

static bool _parseTrue(
    ValueTree& tree,
    const TextContext& ctx,
    PositionInText& pos,
    const Logger& logger
) {
    assert(pos.valid);
    assert(ctx.text[pos.pos] == 't');
    if (ctx.slice(pos, 4) != "true") {
        _logErrorAtPos(
            logger, ctx, pos, "Invalid value. Expected to be \"true\"."
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
    PositionInText& pos,
    const Logger& logger
) {
    assert(pos.valid);
    assert(ctx.text[pos.pos] == 'f');
    if (ctx.slice(pos, 5) != "false") {
        _logErrorAtPos(
            logger, ctx, pos, "Invalid value. Expected to be \"false\"."
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
    PositionInText& pos,
    const Logger& logger
) {
    assert(pos.valid);
    assert(ctx.text[pos.pos] == 'n');
    if (ctx.slice(pos, 4) != "null") {
        _logErrorAtPos(
            logger, ctx, pos, "Invalid value. Expected to be \"null\"."
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
    PositionInText& pos,
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
    _logErrorAtPos(
        logger,
        ctx,
        pos,
        std::string("Invalid JSON value with head: '") + ch + "'."
    );
    return false;
}

ValueTree parse(const std::string& json, const Logger& logger) {
    if (json.empty()) {
        logger.error("Empty JSON.");
        return ValueTree();
    }

    TextContext ctx = { json };
    PositionInText pos = {
        .valid = true, .pos = 0, .lineIdx = 0, .linePos = 0
    };

    ValueTree tree;
    _skipWhitespace(ctx, pos);
    if (!_parseValue(tree, ctx, pos, logger)) {
        logger.error("Failed to parse JSON.");
        return ValueTree();
    }
    _skipWhitespace(ctx, pos);

    if (pos.valid) {
        _logErrorAtPos(logger, ctx, pos, "Extra characters after JSON.");
    }

    return tree;
}

static void _escapeString(const std::string& input, std::stringstream& stream) {
    for (const char c: input) {
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
    const ValueTree& tree,
    std::stringstream& stream,
    bool pretty,
    size_t indent,
    size_t indentStep
) {
    size_t newIndent = indent + indentStep;
    switch (tree.state()) {
        case ValueTree::State::EMPTY: break;

        case ValueTree::State::VALUE: {
            const auto& node = *tree.getValue();
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
            const auto& array = *tree.getArray();
            if (pretty) {
                stream << '[';
                bool first = true;
                bool isEmpty = true;
                for (const auto& value: array) {
                    if (value.isEmpty()) continue;
                    if (first) first = false;
                    else stream << ',';
                    stream << '\n' << std::string(newIndent, ' ');
                    _dump(value, stream, pretty, newIndent, indentStep);
                    isEmpty = false;
                }
                if (!isEmpty) stream << '\n' << std::string(indent, ' ');
                stream << ']';
            } else {
                stream << '[';
                bool first = true;
                for (const auto& value: array) {
                    if (value.isEmpty()) continue;
                    if (first) first = false;
                    else stream << ',';
                    _dump(value, stream, pretty, newIndent, indentStep);
                }
                stream << ']';
            }
        } break;

        case ValueTree::State::OBJECT: {
            const auto& object = *tree.getObject();
            if (pretty) {
                stream << '{';
                bool first = true;
                bool isEmpty = true;
                for (const auto& [key, value]: object) {
                    if (value.isEmpty()) continue;
                    if (first) first = false;
                    else stream << ',';
                    stream << '\n' << std::string(newIndent, ' ');
                    _dump(
                        ValueTree(key), stream, pretty, newIndent, indentStep
                    );
                    stream << ": ";
                    _dump(value, stream, pretty, newIndent, indentStep);
                    isEmpty = false;
                }
                if (!isEmpty) stream << '\n' << std::string(indent, ' ');
                stream << '}';
            } else {
                stream << '{';
                bool first = true;
                for (const auto& [key, value]: object) {
                    if (value.isEmpty()) continue;
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

std::string dump(const ValueTree& tree, bool pretty, size_t indentStep) {
    std::stringstream stream;
    _dump(tree, stream, pretty, 0, indentStep);
    return stream.str();
}

}  // namespace json
}  // namespace c2p
