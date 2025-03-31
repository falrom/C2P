#include "c2p/ini.hpp"

#include "text_utils.hpp"

#include <cassert>
#include <sstream>

namespace c2p {
namespace ini {

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

static bool
_parseWhitespaceInLine(const TextContext& ctx, PositionInText& pos) {
    assert(pos.valid);
    if (ctx.atLineEnd(pos) || !std::isspace(uint8_t(ctx.text[pos.pos]))) {
        return false;
    }
    ctx.moveForwardInLine(pos);
    return true;
}

static bool _parseCommentInLine(const TextContext& ctx, PositionInText& pos) {
    assert(pos.valid);
    if (ctx.atLineEnd(pos)
        || (ctx.text[pos.pos] != ';' && ctx.text[pos.pos] != '#'))
    {
        return false;
    }
    ctx.moveToLineEnd(pos);
    return true;
}

static void _skipWhitespaceInLine(const TextContext& ctx, PositionInText& pos) {
    assert(pos.valid);
    while (_parseWhitespaceInLine(ctx, pos) || _parseCommentInLine(ctx, pos));
}

static std::optional<std::string> _parseQuotedString(
    const TextContext& ctx, PositionInText& pos, const Logger& logger
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
        return std::nullopt;
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
                return std::nullopt;
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
                            return std::nullopt;
                        }
                        if (!std::isxdigit(ctx.text[aheadPos.pos])) {
                            _logErrorAtPos(
                                logger,
                                ctx,
                                aheadPos,
                                "Invalid Unicode escape character. "
                                "Need 4 Hex digits like: \"\\uHHHH\"."
                            );
                            return std::nullopt;
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
                            return std::nullopt;
                        }
                        if (!std::isxdigit(ctx.text[aheadPos.pos])) {
                            _logErrorAtPos(
                                logger,
                                ctx,
                                aheadPos,
                                "Invalid Unicode escape character. "
                                "Need 8 Hex digits like: \"\\UHHHHHHHH\"."
                            );
                            return std::nullopt;
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
                    return std::nullopt;
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
        return std::nullopt;
    }
    ctx.moveForwardInLine(pos);  // Skip closing quote

    return result;
}

static std::optional<std::string> _parseNoQuotedHeaderString(
    const TextContext& ctx, PositionInText& pos, const Logger& logger
) {
    assert(pos.valid);
    assert(!std::isspace(uint8_t(ctx.text[pos.pos])));

    const auto startPos = pos;

    /// Find if there is a whitespace before the closing bracket.
    auto whitespaceStartPos = startPos;

    while (true) {
        if (ctx.text[pos.pos] == ']') break;
        if (ctx.text[pos.pos] == ';' || ctx.text[pos.pos] == '#') {
            _logErrorAtPos(
                logger, ctx, pos, "Unexpected comment in section header."
            );
            return std::nullopt;
        }
        if (ctx.atLineEnd(pos)) {
            _logErrorAtPos(logger, ctx, pos, "No closing bracket found.");
            return std::nullopt;
        }
        if (std::isspace(uint8_t(ctx.text[pos.pos]))) {
            if (whitespaceStartPos.pos == startPos.pos) {
                whitespaceStartPos = pos;
            }
        } else {
            whitespaceStartPos = startPos;
        }
        ctx.moveForwardInLine(pos);
    }

    const std::string result = std::string(ctx.slice(
        startPos,
        whitespaceStartPos.pos == startPos.pos ? pos : whitespaceStartPos
    ));

    return result;
}

static std::optional<std::string> _parseNoQuotedKeyString(
    const TextContext& ctx, PositionInText& pos, const Logger& logger
) {
    assert(pos.valid);
    assert(!std::isspace(uint8_t(ctx.text[pos.pos])));

    const auto startPos = pos;

    /// Find if there is a whitespace before the equal sign.
    auto whitespaceStartPos = startPos;

    while (true) {
        if (ctx.text[pos.pos] == '=') break;
        if (ctx.text[pos.pos] == ';' || ctx.text[pos.pos] == '#') {
            _logErrorAtPos(logger, ctx, pos, "Unexpected comment in key.");
            return std::nullopt;
        }
        if (ctx.atLineEnd(pos)) {
            _logErrorAtPos(logger, ctx, pos, "No '=' found.");
            return std::nullopt;
        }
        if (std::isspace(uint8_t(ctx.text[pos.pos]))) {
            if (whitespaceStartPos.pos == startPos.pos) {
                whitespaceStartPos = pos;
            }
        } else {
            whitespaceStartPos = startPos;
        }
        ctx.moveForwardInLine(pos);
    }

    if (startPos.pos == pos.pos) {
        _logErrorAtPos(
            logger, ctx, startPos, "Empty key without quotes is not allowed."
        );
        return std::nullopt;
    }

    return std::string(ctx.slice(
        startPos,
        whitespaceStartPos.pos == startPos.pos ? pos : whitespaceStartPos
    ));
}

static std::optional<std::string> _parseNoQuotedValueString(
    const TextContext& ctx, PositionInText& pos, const Logger& logger
) {
    assert(pos.valid);
    assert(!std::isspace(uint8_t(ctx.text[pos.pos])));

    const auto startPos = pos;

    /// Find if there is a whitespace before end of line or comment.
    auto whitespaceStartPos = startPos;

    while (true) {
        if (ctx.text[pos.pos] == ';' || ctx.text[pos.pos] == '#') break;
        if (std::isspace(uint8_t(ctx.text[pos.pos]))) {
            // including line break
            if (whitespaceStartPos.pos == startPos.pos) {
                whitespaceStartPos = pos;
            }
        } else {
            whitespaceStartPos = startPos;
        }
        if (!ctx.moveForwardInLine(pos)) {
            if (whitespaceStartPos.pos == startPos.pos) {
                // at line end, but not line break
                whitespaceStartPos = pos;
                whitespaceStartPos.valid = false;
            }
            break;
        }
    }

    return std::string(ctx.slice(
        startPos,
        (whitespaceStartPos.valid && whitespaceStartPos.pos == startPos.pos)
            ? pos
            : whitespaceStartPos
    ));
}

static std::optional<std::string> _parseSectionHeader(
    const TextContext& ctx, PositionInText& pos, const Logger& logger
) {
    assert(pos.valid);
    assert(ctx.text[pos.pos] == '[');

    const auto leftBracketPos = pos;

    if (!ctx.moveForwardInLine(pos)) {  // Skip left bracket
        _logErrorAtPos(
            logger, ctx, leftBracketPos, "Unterminated section header."
        );
        return std::nullopt;
    }
    _skipWhitespaceInLine(ctx, pos);
    if (ctx.text[pos.pos] == ']') {
        _logErrorAtPos(
            logger,
            ctx,
            pos,
            "Empty section header without quotes is not allowed."
        );
        return std::nullopt;
    }
    if (ctx.atLineEnd(pos)) {
        _logErrorAtPos(
            logger, ctx, leftBracketPos, "Unterminated section header."
        );
        return std::nullopt;
    }

    std::optional<std::string> header;
    if (ctx.text[pos.pos] == '"') {
        header = _parseQuotedString(ctx, pos, logger);
        if (!header) {
            _logErrorAtPos(
                logger, ctx, leftBracketPos, "Failed to parse section header."
            );
            return std::nullopt;
        }
    } else {
        header = _parseNoQuotedHeaderString(ctx, pos, logger);
        if (!header) {
            _logErrorAtPos(
                logger, ctx, leftBracketPos, "Failed to parse section header."
            );
            return std::nullopt;
        }
    }

    const auto endPos = pos;
    _skipWhitespaceInLine(ctx, pos);
    if (ctx.text[pos.pos] != ']') {
        _logErrorAtPos(
            logger,
            ctx,
            endPos,
            "Unterminated section header. "
            "Expected ']' at the end of section header."
        );
        return std::nullopt;
    }
    ctx.moveForwardInLine(pos);  // Skip right bracket

    _skipWhitespaceInLine(ctx, pos);
    if (!ctx.atLineEnd(pos)) {
        _logErrorAtPos(
            logger, ctx, pos, "Extra characters after section header."
        );
        return std::nullopt;
    }

    return header;
}

static std::optional<std::pair<std::string, std::string>>
_parseEntry(const TextContext& ctx, PositionInText& pos, const Logger& logger) {
    assert(pos.valid);
    assert(!std::isspace(uint8_t(ctx.text[pos.pos])));

    const auto keyStartPos = pos;
    std::optional<std::string> key;
    if (ctx.text[pos.pos] == '"') {
        key = _parseQuotedString(ctx, pos, logger);
        if (!key) {
            _logErrorAtPos(logger, ctx, keyStartPos, "Failed to parse key.");
            return std::nullopt;
        }
        const auto keyEndPos = pos;
        _skipWhitespaceInLine(ctx, pos);
        if (ctx.text[pos.pos] != '=') {
            _logErrorAtPos(
                logger,
                ctx,
                keyEndPos,
                "Unterminated key. Expected '=' after key."
            );
            return std::nullopt;
        }
    } else {
        key = _parseNoQuotedKeyString(ctx, pos, logger);
        if (!key) {
            _logErrorAtPos(logger, ctx, keyStartPos, "Failed to parse key.");
            return std::nullopt;
        }
    }

    // Skip equal sign
    if (!ctx.moveForwardInLine(pos)) {
        // Empty value
        return std::make_pair(*key, std::string(""));
    }

    _skipWhitespaceInLine(ctx, pos);
    if (ctx.atLineEnd(pos) && std::isspace(uint8_t(ctx.text[pos.pos]))) {
        // Empty value
        return std::make_pair(*key, std::string(""));
    }
    const auto valueStartPos = pos;
    std::optional<std::string> value;
    if (ctx.text[pos.pos] == '"') {
        value = _parseQuotedString(ctx, pos, logger);
        if (!value) {
            _logErrorAtPos(
                logger, ctx, valueStartPos, "Failed to parse value."
            );
            return std::nullopt;
        }
    } else {
        value = _parseNoQuotedValueString(ctx, pos, logger);
        if (!value) {
            _logErrorAtPos(
                logger, ctx, valueStartPos, "Failed to parse value."
            );
            return std::nullopt;
        }
    }

    return std::make_pair(*key, *value);
}

ValueTree parse(const std::string& ini, const Logger& logger) {
    if (ini.empty()) {
        logger.error("Empty INI.");
        return ValueTree();
    }

    TextContext ctx = { ini };
    PositionInText pos = {
        .valid = true, .pos = 0, .lineIdx = 0, .linePos = 0
    };

    ValueTree tree;
    ValueTree* section = &tree;
    do {

#if false
        logger.info(
            "Parsing Line: " + std::to_string(pos.lineIdx) + " \""
            + std::string(
                ctx.slice(pos, ctx.lines[pos.lineIdx].lenExcludingBreaks)
            )
            + '"'
        );
#endif

        _skipWhitespaceInLine(ctx, pos);
        if (ctx.atLineEnd(pos) && std::isspace(uint8_t(ctx.text[pos.pos])))
            continue;

        const auto lineStartPos = pos;
        if (ctx.text[pos.pos] == '[') {
            auto header = _parseSectionHeader(ctx, pos, logger);
            if (!header) {
                logger.error(
                    lineStartPos.toString() + ": Failed to parse section."
                );
                return ValueTree();
            }
            section = &(tree[*header]);
            section->asObject();
        } else {
            auto entry = _parseEntry(ctx, pos, logger);
            if (!entry) {
                logger.error(
                    lineStartPos.toString() + ": Failed to parse entry."
                );
                return ValueTree();
            }
            (*section)[entry->first] = entry->second;
        }
    } while (ctx.moveToNextLine(pos));

    return tree;
}

static void _dumpString(const std::string& str, std::stringstream& stream) {
    if (str.empty()) {
        stream << "\"\"";
        return;
    }
    std::string result;
    bool needQuote = false;
    for (const char c: str) {
        switch (c) {
            case ';': {
                result.push_back(c);
                needQuote = true;
            } break;
            case '#': {
                result.push_back(c);
                needQuote = true;
            } break;
            case '"': {
                result += ("\\\"");
                needQuote = true;
            } break;
            case '\\': {
                result += ("\\\\");
                needQuote = true;
            } break;
            case '\b': {
                result += ("\\b");
                needQuote = true;
            } break;
            case '\f': {
                result += ("\\f");
                needQuote = true;
            } break;
            case '\n': {
                result += ("\\n");
                needQuote = true;
            } break;
            case '\r': {
                result += ("\\r");
                needQuote = true;
            } break;
            case '\t': {
                result += ("\\t");
                needQuote = true;
            } break;
            default: {
                result.push_back(c);
            } break;
        }
    }
    if (needQuote) stream << '"';
    stream << result;
    if (needQuote) stream << '"';
}

static void
_dumpSectionHeader(const std::string& header, std::stringstream& stream) {
    stream << '[';
    _dumpString(header, stream);
    stream << "]\n";
}

static void _dumpEntry(
    const std::string& key, const ValueNode& node, std::stringstream& stream
) {
    _dumpString(key, stream);
    stream << '=';
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
            _dumpString(*node.value<TypeTag::STRING>(), stream);
        } break;
    }
    stream << '\n';
}

std::string dump(const ValueTree& tree) {
    if (tree.isEmpty()) return "";
    if (!tree.isObject()) return "";

    std::stringstream stream;

    const auto& object = *tree.getObject();
    std::map<std::string, const ObjectNode*> sections;

    // dump global entries & collect sections
    for (const auto& [key, value]: object) {
        if (value.isEmpty()) continue;
        if (value.isArray()) return "";  // INI does not support array
        if (value.isObject()) {
            sections[key] = value.getObject();
            continue;
        }
        assert(value.isValue());
        _dumpEntry(key, *value.getValue(), stream);
    }

    // dump sections
    for (const auto& [header, entries]: sections) {
        stream << '\n';
        _dumpSectionHeader(header, stream);
        for (const auto& [key, value]: *entries) {
            if (value.isEmpty()) continue;
            if (!value.isValue()) return "";  // must be key-value entry
            _dumpEntry(key, *value.getValue(), stream);
        }
    }

    return stream.str();
}

}  // namespace ini
}  // namespace c2p
