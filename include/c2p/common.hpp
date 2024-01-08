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

    using LogCallback = std::function<void(const std::string& logStr)>;

    LogCallback logErrorCallback;
    LogCallback logWarningCallback;
    LogCallback logInfoCallback;

    Logger(
        LogCallback logErrorCallback = nullptr,
        LogCallback logWarningCallback = nullptr,
        LogCallback logInfoCallback = nullptr
    )
        : logErrorCallback(logErrorCallback),
          logWarningCallback(logWarningCallback),
          logInfoCallback(logInfoCallback) {}

    // clang-format off
    void logError  (const std::string& logStr) const { if (logErrorCallback  ) { logErrorCallback(logStr);   } }
    void logWarning(const std::string& logStr) const { if (logWarningCallback) { logWarningCallback(logStr); } }
    void logInfo   (const std::string& logStr) const { if (logInfoCallback   ) { logInfoCallback(logStr);    } }
    // clang-format on
};

}  // namespace c2p

#endif  // __C2P_COMMON_HPP__
