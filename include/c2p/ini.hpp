/**
 * @file ini.hpp
 * @brief INI serialization and deserialization.
 * Based on value tree.
 */

#ifndef __C2P_INI_HPP__
#define __C2P_INI_HPP__

#include <c2p/common.hpp>
#include <c2p/value_tree.hpp>

namespace c2p {
namespace ini {

// TODO

#if false
/// Pase INI string into value tree.
/// If the input INI string is invalid, return NONE.
///
/// Extended INI Grammar:
/// - Allow single-line comments starting with ';' or '#'.
/// - Allow no-section key-value pairs at the beginning.
value_tree::ValueNode
parse(const std::string& ini, const Logger& logger = Logger());

/// Serialize value tree into INI string.
///
/// Note: Since INI files do NOT support arrays and complex nesting, if the
/// input node can NOT be converted to a valid INI format, an empty string will
/// be returned.
std::string dump(value_tree::ValueNode& node);
#endif

}  // namespace ini
}  // namespace c2p

#endif  // __C2P_INI_HPP__
