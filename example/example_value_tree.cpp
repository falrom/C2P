#include <c2p/json.hpp>
#include <c2p/value_tree.hpp>
#include <iostream>
#include <optional>
#include <vector>

using namespace c2p::value_tree;

const c2p::Logger logger{
    // clang-format off
    [](const std::string& logStr) { std::cerr << "Error: "   << logStr << std::endl; },
    [](const std::string& logStr) { std::cout << "Warning: " << logStr << std::endl; },
    [](const std::string& logStr) { std::cout << "Info: "    << logStr << std::endl; },
    // clang-format on
};

int main(int argc, char* argv[]) {

    ValueNode nodeNumber1(42);
    std::cout << "nodeNumber1: type = " << int(nodeNumber1.type())
              << ", value = " << (*nodeNumber1.value<ValueType::NUMBER>())
              << std::endl;

    ValueNode nodeNumber2 = 3.1415926;
    std::cout << "nodeNumber2: type = " << int(nodeNumber2.type())
              << ", value = " << (*nodeNumber2.value<ValueType::NUMBER>())
              << std::endl;

    ValueNode nodeNone(NONE);
    std::cout << "nodeNone: type = " << int(nodeNone.type()) << ", value = "
              << ((*nodeNone.value<ValueType::NONE>() == NONE) ? "NONE"
                                                               : "NOT NONE")
              << std::endl;

    ValueNode nodeString = "sadfasfsadfas";
    std::cout << "nodeString: type = " << int(nodeString.type())
              << ", value = " << (*nodeString.value<ValueType::STRING>())
              << std::endl;

    ValueNode nodeBool = true;
    std::cout << "nodeBool: type = " << int(nodeBool.type())
              << ", value = " << (*nodeBool.value<ValueType::BOOL>())
              << std::endl;

    auto nodeArray = ValueNode::from(std::vector{ 2, 3, 4, 5 });
    std::cout << "nodeArray: type = " << int(nodeArray.type())
              << ", value = " << c2p::json::dump(nodeArray) << std::endl;

    auto nodeObject = ValueNode::from(std::map<std::string, int>{
        { "1", 1 },
        { "2", 2 },
        { "3", 3 },
    });
    std::cout << "nodeObject: type = " << int(nodeObject.type())
              << ", value = " << c2p::json::dump(nodeObject) << std::endl;

    ValueNode node;
    node["aaa"]["bbb"]["ccc"]["ddd"] = 9;
    node["aaa"]["bbb"]["ddd"] = nodeNumber2;
    node["aaa"]["bbb"]["eee"].asArray().emplace_back(NONE);
    auto& array = node["aaa"]["bbb"]["eee"].asArray();
    array.emplace_back(nodeArray);
    array.emplace_back(nodeObject);
    auto& sensorConfig = node["sensors"];
    sensorConfig["sensor1"]["name"] = "sensor1";
    sensorConfig["sensor1"]["type"] = "temperature";
    sensorConfig["sensor1"]["value"] = 26;
    sensorConfig["sensor2"]["name"] = "sensor2";
    sensorConfig["sensor2"]["type"] = "height";
    sensorConfig["sensor2"]["value"] = 42.0;
    std::cout << "node: " << c2p::json::dump(node, true, 4) << std::endl;
}
