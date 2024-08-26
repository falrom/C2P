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

    const std::string json = R"(
    ////////allow comment
{
    "numbers": [ 1, 2, 3, 4, 5, 6, 7, 8, 9, 10
        , +11,  // allow '+' for positive numbers
        -12, 13.14, 15.16e+17, 18.19e-20
    ],
    "lidar_2d"    : { "enable": true },
    "lidar_3d"    : { "enable": true },
    "rgbd"        : { "enable": true },
    "array": [ "sadfsafs", 
    "asdfasdf\nsadfsadffsadf"
    ,      // allow trailing comma
    ],  // allow trailing comma
    // allow comment
}
)";
    auto nn = c2p::json::parse(json, logger);
    std::cout << c2p::json::dump(nn, true, 4) << std::endl;
}
