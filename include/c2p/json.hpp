/**
 * @file json.hpp
 * @brief JSON serialization and deserialization.
 * Based on value tree.
 */

#ifndef __C2P_JSON_HPP__
#define __C2P_JSON_HPP__

#include <c2p/common.hpp>
#include <c2p/value_tree.hpp>

namespace c2p {
namespace json {

/// Pase JSON string into value tree.
/// If the input JSON string is invalid, return NONE.
///
/// Extended JSON Grammar:
/// - trailing comma is allowed
/// - '+' is allowed for positive numbers
/// - single-line comment starts with '//'
value_tree::ValueNode
parse(const std::string& json, const Logger& logger = Logger());

/// Serialize value tree into JSON string.
/// If valueNode is NONE, return empty string.
std::string
dump(value_tree::ValueNode& node, bool pretty = false, size_t indentStep = 2);

}  // namespace json
}  // namespace c2p

#endif  // __C2P_JSON_HPP__
