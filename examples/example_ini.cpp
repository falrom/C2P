#include <c2p/ini.hpp>
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

    const std::string ini = R"(
; comment starts with ';'
# comment starts with '#'

; allow no-section key-value pairs at the beginning

name=John Doe
age=  30
city  =  New York   

[ section 1 ] ; allow spaces before and after section header string
email = "name\u0040fake.com" ; same as "name@fake.com"
home addr = 银河系 - 太阳系 - 地球: 北极点   

[""] ; allow quoted string as empty section
empty info =
empty info2 =  ; allow empty value string even without quotes

; comment starts with ';'
# comment starts with '#'

"" = value of empty key ; allow quoted string as empty key)";

    auto tree = c2p::ini::parse(ini, logger);
    std::cout << "---------- JSON ----------" << std::endl;
    std::cout << c2p::json::dump(tree, true, 4) << std::endl;
    std::cout << "---------- INI ----------" << std::endl;
    std::cout << c2p::ini::dump(tree) << std::endl;
}
