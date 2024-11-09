#include <c2p/c2p.hpp>
#include <iostream>
#include <optional>
#include <vector>

struct MyConfig: public c2p::Config {
    std::optional<double> cA;
    std::optional<double> cB;
    std::optional<std::string> cC;
};

struct MyParam: public c2p::Param {
    int pAxB;
    std::string pC;
};

const c2p::Logger logger{
    // clang-format off
    [](const std::string& logStr) { std::cerr << "Error: "   << logStr << std::endl; },
    [](const std::string& logStr) { std::cout << "Warning: " << logStr << std::endl; },
    [](const std::string& logStr) { std::cout << "Info: "    << logStr << std::endl; },
    // clang-format on
};

int main(int argc, char* argv[]) {

    MyConfig myConfig;
    MyParam myParam;

    myConfig.cA = 10.0;
    myConfig.cB = 3.3;
    myConfig.cC = "thisIsAnEmail@test.com";

    const auto rule1 = c2p::Rule{
        .description = "pAxB = cA times cB as int.",
        .transform =
            [](auto& config, auto& param, auto& logger) {
                auto& myConfig = static_cast<const MyConfig&>(config);
                auto& myParam = static_cast<MyParam&>(param);
                if (!myConfig.cA) {
                    logger.error("cA was not set.");
                    return false;
                }
                if (!myConfig.cB) {
                    logger.error("cB was not set.");
                    return false;
                }
                myParam.pAxB = int((*myConfig.cA) * (*myConfig.cB));
                return true;
            },
    };

    const auto rule2 = c2p::Rule{
        .description = "cC must not be empty.",
        .transform =
            [](auto& config, auto& param, auto& logger) {
                auto& myConfig = static_cast<const MyConfig&>(config);
                auto& myParam = static_cast<MyParam&>(param);
                if (!myConfig.cC) {
                    logger.error("cC was not set.");
                    return false;
                }
                if ((*myConfig.cC).empty()) {
                    logger.error("cC was empty.");
                    return false;
                }
                myParam.pC = *myConfig.cC;
                return true;
            },
    };

    if (c2p::doTransform(myConfig, myParam, { rule1, rule2 }, logger)) {
        std::cout << "Transformed successfully." << std::endl;
        return EXIT_SUCCESS;
    } else {
        return EXIT_FAILURE;
    }
}
