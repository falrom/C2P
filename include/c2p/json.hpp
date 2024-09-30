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

/// Parse JSON string into ValueTree.
///
/// If the input JSON string is invalid, return an empty ValueTree.
///
/// Extended JSON Grammar:
/// - Allow trailing comma in arrays and objects.
/// - Allow '+' sign for positive numbers.
/// - Allow single-line comment starts with '//'.
ValueTree parse(const std::string& json, const Logger& logger = Logger());

/// Serialize ValueTree into JSON string.
///
/// If ValueTree is empty, return an empty string.
/// If some subtrees are empty, they will not be serialized.
std::string dump(ValueTree& tree, bool pretty = false, size_t indentStep = 2);

}  // namespace json
}  // namespace c2p

#endif  // __C2P_JSON_HPP__
