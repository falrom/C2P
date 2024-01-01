#include <c2p/c2p.hpp>
#include <iostream>
#include <vector>

struct MyConfig: public c2p::Config {
    int         ca;
    double      cb;
    std::string cc;
};

struct MyParam: public c2p::Param {
    double      pa;
    double      pb;
    std::string pc;
};

int main(int argc, char* argv[]) {

    MyConfig config;
    config.ca = 10;
    config.cb = 1.0;
    config.cc = "2.0";

    MyParam param;

    auto rule1 = c2p::Rule{};
    rule1.description = "Check ca, should be 0.",
    rule1.transform = [](auto config, auto param, auto logger) {
        const auto& myConfig = static_cast<const MyConfig&>(config);
        return myConfig.ca == 0;
    };

    c2p::Logger logger;
    logger.logErrorCallback = [](const std::string& logStr) {
        std::cerr << "Error: " << logStr << std::endl;
    };
    logger.logWarningCallback = [](const std::string& logStr) {
        std::cout << "Warning: " << logStr << std::endl;
    };
    logger.logInfoCallback = [](const std::string& logStr) {
        std::cout << "Info: " << logStr << std::endl;
    };

    if (c2p::doTransform(config, param, { rule1 }, logger)) {
        return 0;
    } else {
        return 1;
    }
}
