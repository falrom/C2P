#include <c2p/json.hpp>
#include <iostream>
#include <optional>
#include <vector>

const c2p::Logger logger{
    // clang-format off
    [](const std::string& logStr) { std::cerr << "Error: "   << logStr << std::endl; },
    [](const std::string& logStr) { std::cout << "Warning: " << logStr << std::endl; },
    [](const std::string& logStr) { std::cout << "Info: "    << logStr << std::endl; },
    // clang-format on
};

int main(int argc, char* argv[]) {

    logger.info(std::string("Project Version: v") + c2p::ProjectVersion);
    logger.info(std::string("Project Git Commit: ") + c2p::ProjectGitCommit);
    logger.info(std::string("Project Git Branch: ") + c2p::ProjectGitBranch);
    logger.info(std::string("Project CMake Time: ") + c2p::ProjectCmakeTime);
    logger.info(std::string("Project Build Time: ") + c2p::ProjectBuildTime);

    const std::string json = R"(
    // allow comment
{
    "numbers": [ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10
        , +11,  // allow '+' for positive numbers
        -12, 13.14, 15.16e+17, 18.19e-20
    ],
    "sensor1": { "enable": true },
    "sensor2": { "enable": false },
    "sensor3": { "enable": true },
    "array": [ "sadfsafs",    
    "asdfasdf\nsadfsadffsadf\u0040"
    ,  // allow trailing comma
    ],  // allow trailing comma
    // allow comment
}
)";

    auto tree = c2p::json::parse(json, logger);
    std::cout << c2p::json::dump(tree, true, 4) << std::endl;
}
