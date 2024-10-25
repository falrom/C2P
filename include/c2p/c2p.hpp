/**
 * @file c2p.hpp
 */

#ifndef __C2P_C2P_HPP__
#define __C2P_C2P_HPP__

#include "c2p/common.hpp"

namespace c2p {

/// Configuration base class definition.
struct Config {
    Config() = default;
    virtual ~Config() = default;
};

/// Parameter base class definition.
struct Param {
    Param() = default;
    virtual ~Param() = default;
};

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

/// Transform config into param by applying all rules in order.
inline bool doTransform(
    const Config& config,
    Param& param,
    const std::vector<Rule>& rules,
    const Logger& logger = Logger()
) {
    for (const auto& rule: rules) {
        if (!rule.transform) {
            logger.warning(
                "Empty rule with description: \"" + rule.description + "\""
            );
            continue;
        }
        if (!rule.transform(config, param, logger)) {
            logger.error(
                "Rule failed with description: \"" + rule.description + "\""
            );
            return false;
        }
    }
    return true;
}

}  // namespace c2p

#endif  // __C2P_C2P_HPP__
