/**
 * @file ini.hpp
 * @brief INI serialization and deserialization.
 * Based on ValueTree.
 */

#ifndef __C2P_INI_HPP__
#define __C2P_INI_HPP__

#include <c2p/common.hpp>
#include <c2p/value_tree.hpp>

namespace c2p {
namespace ini {

/// Parse INI string into ValueTree.
///
/// If the input INI string is invalid, return an empty ValueTree.
///
/// Extended INI Grammar:
/// - Allow single-line comments starting with ';' or '#'.
/// - Allow no-section key-value pairs at the beginning.
/// - Allow empty value string even without quotes.
ValueTree parse(const std::string& ini, const Logger& logger = Logger());

/// Serialize ValueTree into INI string.
///
/// If ValueTree is empty, return an empty string.
/// If some subtrees are empty, they will not be serialized.
///
/// Note: Since INI files do NOT support arrays and complex nesting, if the
/// input tree can NOT be converted to a valid INI format, an empty string will
/// be returned.
std::string dump(const ValueTree& tree);

}  // namespace ini
}  // namespace c2p

#endif  // __C2P_INI_HPP__
