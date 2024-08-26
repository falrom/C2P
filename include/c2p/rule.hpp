/**
 * @file rule.hpp
 * @brief Rule defination, which is used
 * to transform something from config to param.
 */

#ifndef __C2P_RULE_HPP__
#define __C2P_RULE_HPP__

#include "c2p/common.hpp"
#include "c2p/config.hpp"
#include "c2p/param.hpp"

namespace c2p {

/// Define a rule to transform something from config to param.
///
/// You can perform only part of the transformation in one rule,
/// and then complete the entire transformation by combining multiple rules.
///
/// Of course, you can also define a rule to only perform conditional checks
/// without changing param. Returns false if config fails the check.
struct Rule {

    /// To complete the partial transformation from config to param.
    /// @param[in] config The config to transform.
    /// @param[out] param The param to transform to.
    /// @param logger The log tool callbacks.
    /// @return True if the transformation was successful, false otherwise.
    using TransformCallback = std::function<
        bool(const Config& config, Param& param, const Logger& logger)>;

    /// Add your description of this rule here.
    std::string description;

    /// Will call this function to complete the partial transformation from
    /// config to param. See more details in `TransformCallback`.
    TransformCallback transform;
};

}  // namespace c2p

#endif  // __C2P_RULE_HPP__
