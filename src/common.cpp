#include "c2p/common.hpp"

namespace c2p {

#ifdef PROJECT_VERSION
const std::string ProjectVersion = PROJECT_VERSION;
#else
const std::string ProjectVersion = "";
#endif

#ifdef PROJECT_GIT_COMMIT
const std::string ProjectGitCommit = PROJECT_GIT_COMMIT;
#else
const std::string ProjectGitCommit = "";
#endif

#ifdef PROJECT_GIT_BRANCH
const std::string ProjectGitBranch = PROJECT_GIT_BRANCH;
#else
const std::string ProjectGitBranch = "";
#endif

#ifdef PROJECT_CMAKE_TIME
const std::string ProjectCmakeTime = PROJECT_CMAKE_TIME;
#else
const std::string ProjectCmakeTime = "";
#endif

const std::string ProjectBuildTime = __DATE__ " " __TIME__;

}  // namespace c2p
