#include "c2p/json.hpp"

#include <sstream>

using namespace c2p::value_tree;

namespace c2p {
namespace json {

ValueNode parse(const std::string& json) {
    ValueNode node;
    // TODO
    return node;
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
            // TODO: escape
            stream << '"' << *node.value<ValueType::STRING>() << '"';
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
