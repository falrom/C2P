/**
 * @file text_utils.hpp
 * @brief Text utilities.
 */

#ifndef __C2P_TEXT_UTILS_HPP__
#define __C2P_TEXT_UTILS_HPP__

#include <string>
#include <vector>

namespace c2p {

/// Describes a character position in a text.
struct TextPosition {
    bool valid;

    uint32_t pos;
    uint32_t lineIdx;
    uint32_t linePos;

    std::string toString() const {
        return "line:" + std::to_string(lineIdx + 1) + ":"
             + std::to_string(linePos + 1);
    }
};

/// Describes a line in a text.
struct TextLine {
    uint32_t pos;  ///< Position of the first character in the line.
    uint32_t len;  ///< Length of the line (including line ending character).
};

/// Split text into lines.
/// Allowing for '\n', '\r', and '\r\n' as line endings.
std::vector<TextLine> splitLines(const std::string& text) {
    std::vector<TextLine> lines;
    uint32_t pos = 0;
    uint32_t len = 0;
    for (char c: text) {
        ++len;
        if (c == '\n') {
            lines.push_back({ pos, len });
            pos += len;
            len = 0;
        } else if (c == '\r') {
            const uint32_t newPos = pos + len;
            if (newPos < text.size() && text[newPos] == '\n') {
                ++len;
            }
            lines.push_back({ pos, len });
            pos += len;
            len = 0;
        }
    }
    if (len > 0) {
        lines.push_back({ pos, len });
    }
    return lines;
}

/// Describes a text context.
/// Provides access to the original text and its lines table.
struct TextContext {
    const std::string& text;
    const std::vector<TextLine> lines;

    TextContext(const std::string& text): text(text), lines(splitLines(text)) {}

    /// Move the position forward by one character.
    /// If the new position is out of the text:
    /// - the position will not be moved.
    /// - the position will be marked as invalid.
    /// - returns false.
    bool moveForward(TextPosition& pos) const {
        if (!pos.valid) {
            return false;
        }
        if (pos.linePos + 1 < lines[pos.lineIdx].len) {
            ++pos.linePos;
            ++pos.pos;
            return true;
        }
        if (pos.lineIdx + 1 < lines.size()) {
            ++pos.lineIdx;
            pos.pos = lines[pos.lineIdx].pos;
            pos.linePos = 0;
            return true;
        }
        pos.valid = false;
        return false;
    }

    /// Move the position forward by a number of characters.
    /// If the new position is out of the text:
    /// - the position will be moved as far as possible.
    /// - the position will be marked as invalid.
    /// - returns false.
    bool moveForward(TextPosition& pos, uint32_t count) const {
        for (uint32_t step = 0; step < count; ++step) {
            if (!moveForward(pos)) {
                return false;
            }
        }
        return true;
    }

    /// Move the position forward to the start of the next line.
    /// If there is no next line:
    /// - the position will be moved to the end of the text.
    /// - the position will be marked as invalid.
    /// - returns false.
    bool moveNextLine(TextPosition& pos) const {
        if (!pos.valid) {
            return false;
        }
        if (pos.lineIdx + 1 < lines.size()) {
            ++pos.lineIdx;
            pos.pos = lines[pos.lineIdx].pos;
            pos.linePos = 0;
            return true;
        }
        pos.pos = text.size() - 1;
        pos.linePos = lines[pos.lineIdx].len - 1;
        pos.valid = false;
        return false;
    }

    /// Get slice of text from the start position with a given length.
    std::string_view slice(const TextPosition& start, uint32_t len) const {
        if (!start.valid) return std::string_view("");
        return std::string_view(text.data() + start.pos, len);
    }

    /// Get slice of text from the start position to the end position
    /// (exclusive).
    std::string_view
    slice(const TextPosition& start, const TextPosition& end) const {
        if (!start.valid) return std::string_view("");
        if (!end.valid) return std::string_view(text.data() + start.pos);
        return std::string_view(text.data() + start.pos, end.pos - start.pos);
    }
};

}  // namespace c2p

#endif  // __C2P_TEXT_UTILS_HPP__