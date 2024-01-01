/**
 * @file common.hpp
 * @brief Common definations.
 */

#ifndef __C2P_COMMON_HPP__
#define __C2P_COMMON_HPP__

#include <functional>
#include <string>

namespace c2p {

struct Logger {
    std::function<void(const std::string&)> logErrorCallback;
    std::function<void(const std::string&)> logWarningCallback;
    std::function<void(const std::string&)> logInfoCallback;
    // clang-format off
    void logError  (const std::string& logStr) const { if (logErrorCallback  ) { logErrorCallback(logStr);   } }
    void logWarning(const std::string& logStr) const { if (logWarningCallback) { logWarningCallback(logStr); } }
    void logInfo   (const std::string& logStr) const { if (logInfoCallback   ) { logInfoCallback(logStr);    } }
    // clang-format on
};

}  // namespace c2p

#endif  // __C2P_COMMON_HPP__
