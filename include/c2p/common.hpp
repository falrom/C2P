/**
 * @file common.hpp
 * @brief Common definations.
 */

#ifndef __C2P_COMMON_HPP__
#define __C2P_COMMON_HPP__

#include <functional>
#include <string>

namespace c2p {

// ANSI format escape sequences.
#define ANSI_FMT_RESET         "\033[0m"  /* Default */
#define ANSI_FMT_BOLD          "\033[1m"  /* Bold */
#define ANSI_FMT_BLACK         "\033[30m" /* Black */
#define ANSI_FMT_RED           "\033[31m" /* Red */
#define ANSI_FMT_GREEN         "\033[32m" /* Green */
#define ANSI_FMT_YELLOW        "\033[33m" /* Yellow */
#define ANSI_FMT_BLUE          "\033[34m" /* Blue */
#define ANSI_FMT_MAGENTA       "\033[35m" /* Magenta */
#define ANSI_FMT_CYAN          "\033[36m" /* Cyan */
#define ANSI_FMT_WHITE         "\033[37m" /* White */
#define ANSI_FMT_BRIGHTBLACK   "\033[90m" /* Bright Black */
#define ANSI_FMT_BRIGHTRED     "\033[91m" /* Bright Red */
#define ANSI_FMT_BRIGHTGREEN   "\033[92m" /* Bright Green */
#define ANSI_FMT_BRIGHTYELLOW  "\033[93m" /* Bright Yellow */
#define ANSI_FMT_BRIGHTBLUE    "\033[94m" /* Bright Blue */
#define ANSI_FMT_BRIGHTMAGENTA "\033[95m" /* Bright Magenta */
#define ANSI_FMT_BRIGHTCYAN    "\033[96m" /* Bright Cyan */
#define ANSI_FMT_BRIGHTWHITE   "\033[97m" /* Bright White */

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
    void error  (const std::string& logStr) const { if (logErrorCallback  ) { logErrorCallback(logStr);   } }
    void warning(const std::string& logStr) const { if (logWarningCallback) { logWarningCallback(logStr); } }
    void info   (const std::string& logStr) const { if (logInfoCallback   ) { logInfoCallback(logStr);    } }
    // clang-format on
};

extern const std::string ProjectVersion;
extern const std::string ProjectGitCommit;
extern const std::string ProjectGitBranch;
extern const std::string ProjectCmakeTime;
extern const std::string ProjectBuildTime;

}  // namespace c2p

#endif  // __C2P_COMMON_HPP__
