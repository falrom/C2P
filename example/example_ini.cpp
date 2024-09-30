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

    const std::string ini = R"(
; comment starts with ';'
# comment starts with '#'

; allow no-section key-value pairs at the beginning

name=John Doe
age=  30
city  =  New York   

[ section 1 ] ; allow spaces before and after section header string
email = "name\u0040fake.com"
home addr = 银河系 - 太阳系 - 地球: 北极点   

[""] ; allow quoted string as empty section
empty info =
empty info2 =  ; allow empty value string even without quotes

; comment starts with ';'
# comment starts with '#'

"" = value of empty key ; allow quoted string as empty key)";

    auto node = c2p::ini::parse(ini, logger);
    std::cout << c2p::json::dump(node, true, 4) << std::endl;
}