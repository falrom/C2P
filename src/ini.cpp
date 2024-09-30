#include "c2p/ini.hpp"

#include "text_utils.hpp"

#include <cassert>

namespace c2p {
namespace ini {

static void _logErrorAtPos(
    const Logger& logger,
    const TextContext& ctx,
    const PositionInText& pos,
    const std::string& msg
) {
    logger.logError(
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

    if (!ctx.moveForwardInLine(pos)) {
        _logErrorAtPos(
            logger, ctx, leftQuotePos, "Unterminated quoted string."
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
                    if (!ctx.moveForwardInLine(aheadPos, 4)) {
                        _logErrorAtPos(
                            logger, ctx, pos, "Invalid Unicode escape sequence."
                        );
                        return std::nullopt;
                    }
                    ctx.moveForwardInLine(pos);
                    result.push_back(static_cast<char>(
                        std::stoi(std::string(ctx.slice(pos, 4)), nullptr, 16)
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
        _logErrorAtPos(logger, ctx, leftQuotePos, "Unterminated string.");
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
    assert(!ctx.atLineEnd(pos));

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
    assert(!ctx.atLineEnd(pos));

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
            if (whitespaceStartPos.pos == startPos.pos) {
                whitespaceStartPos = pos;
            }
        } else {
            whitespaceStartPos = startPos;
        }
        if (!ctx.moveForwardInLine(pos)) break;
    }

    return std::string(ctx.slice(
        startPos,
        whitespaceStartPos.pos == startPos.pos ? pos : whitespaceStartPos
    ));
}

static std::optional<std::string> _parseSectionHeader(
    const TextContext& ctx, PositionInText& pos, const Logger& logger
) {
    assert(pos.valid);
    assert(ctx.text[pos.pos] == '[');
    assert(!ctx.atLineEnd(pos));

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

    const auto startPos = pos;
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
    assert(!ctx.atLineEnd(pos));

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

    ctx.moveForwardInLine(pos);  // Skip equal sign

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
        logger.logError("Empty INI.");
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
        logger.logInfo(
            "Parsing Line: " + std::to_string(pos.lineIdx) + " \""
            + std::string(
                ctx.slice(pos, ctx.lines[pos.lineIdx].lenWithoutBreaks)
            )
            + '"'
        );
#endif

        _skipWhitespaceInLine(ctx, pos);
        if (ctx.atLineEnd(pos)) continue;

        const auto lineStartPos = pos;
        if (ctx.text[pos.pos] == '[') {
            auto header = _parseSectionHeader(ctx, pos, logger);
            if (!header) {
                logger.logError(
                    lineStartPos.toString() + ": Failed to parse section."
                );
                return ValueTree();
            }
            section = &(tree[*header]);
        } else {
            auto entry = _parseEntry(ctx, pos, logger);
            if (!entry) {
                logger.logError(
                    lineStartPos.toString() + ": Failed to parse entry."
                );
                return ValueTree();
            }
            (*section)[entry->first] = entry->second;
        }
    } while (ctx.moveToNextLine(pos));

    return tree;
}

}  // namespace ini
}  // namespace c2p
