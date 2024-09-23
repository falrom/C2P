/**
 * @file json.hpp
 * @brief JSON serialization and deserialization.
 * Based on ValueTree.
 */

#ifndef __C2P_JSON_HPP__
#define __C2P_JSON_HPP__

#include <c2p/common.hpp>
#include <c2p/value_tree.hpp>

namespace c2p {
namespace json {

/// Pase JSON string into ValueTree.
/// If the input JSON string is invalid, return an empty ValueTree.
///
/// Extended JSON Grammar:
/// - trailing comma is allowed
/// - '+' is allowed for positive numbers
/// - single-line comment starts with '//'
ValueTree parse(const std::string& json, const Logger& logger = Logger());

/// Serialize ValueTree into JSON string.
///
/// If ValueTree is empty, return an empty string.
std::string dump(ValueTree& node, bool pretty = false, size_t indentStep = 2);

}  // namespace json
}  // namespace c2p

#endif  // __C2P_JSON_HPP__
