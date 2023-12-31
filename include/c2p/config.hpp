/**
 * @file config.hpp
 * @brief Configuration base class definition.
 */

#ifndef __C2P_CONFIG_HPP__
#define __C2P_CONFIG_HPP__

namespace c2p {

struct Config {
    Config() = default;
    virtual ~Config() = default;
};

}  // namespace c2p

#endif  // __C2P_CONFIG_HPP__
