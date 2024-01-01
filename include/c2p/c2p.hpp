/**
 * @file c2p.hpp
 */

#ifndef __C2P_C2P_HPP__
#define __C2P_C2P_HPP__

#include "c2p/rule.hpp"

namespace c2p {

inline bool doTransform(
    const Config&            config,
    Param&                   param,
    const std::vector<Rule>& rules,
    const Logger&            logger = Logger()
) {
    for (const auto& rule: rules) {
        if (!rule.transform) {
            logger.logWarning("Got an empty rule: " + rule.description);
            continue;
        }
        if (!rule.transform(config, param, logger)) {
            logger.logError("Rule transform failed: " + rule.description);
            return false;
        }
    }
    return true;
}

}  // namespace c2p

#endif  // __C2P_C2P_HPP__
