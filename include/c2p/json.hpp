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
/// Extended JSON PEG Grammar:
/// - trailing comma is allowed
/// - '+' is allowed for positive numbers
/// - single-line comment starts with '//'
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
///
/// Character  <- !["\\] .
///            / '\\' Escape
/// Escape     <- ["\\/bfnrt]
///            / 'u' Hex Hex Hex Hex
///
/// Number     <- ('+' / '-')? Integer Fraction? Exponent?  // '+' is OK
/// for positive numbers Integer    <- '0' / [1-9] [0-9]* Fraction   <- '.'
/// [0-9]+ Exponent   <- [eE] [+-]? [0-9]+
///
/// S          <- (Whitespace / Comment)*
/// Whitespace <- [ \t\n\r]
/// Comment    <- '//' (![\n\r] .)*  // single-line comment starts with '//'
///
/// Hex        <- [0-9a-fA-F]
/// ```
value_tree::ValueNode
parse(const std::string& json, const Logger& logger = Logger());

/// Serialize value tree into JSON string.
/// If valueNode is NONE, return empty string.
std::string
dump(value_tree::ValueNode& node, bool pretty = false, size_t indentStep = 2);

}  // namespace json
}  // namespace c2p

#endif  // __C2P_JSON_HPP__
