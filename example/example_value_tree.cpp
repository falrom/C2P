#include <c2p/json.hpp>
#include <c2p/value_tree.hpp>
#include <iostream>
#include <optional>
#include <vector>

using namespace c2p;

const Logger logger{
    // clang-format off
    [](const std::string& logStr) { std::cerr << "Error: "   << logStr << std::endl; },
    [](const std::string& logStr) { std::cout << "Warning: " << logStr << std::endl; },
    [](const std::string& logStr) { std::cout << "Info: "    << logStr << std::endl; },
    // clang-format on
};

int main(int argc, char* argv[]) {

    ValueTree treeNone(NONE);
    std::cout << "treeNone: state = " << int(treeNone.state()) << ", value = "
              << ((*treeNone.value<TypeTag::NONE>() == NONE) ? "NONE"
                                                             : "NOT NONE")
              << std::endl;

    ValueTree treeBool(true);
    std::cout << "treeBool: state = " << int(treeBool.state()) << ", value = "
              << (*treeBool.value<TypeTag::BOOL>() ? "true" : "false")
              << std::endl;

    ValueTree treeNumber1(42);
    std::cout << "treeNumber1: state = " << int(treeNumber1.state())
              << ", value = " << (*treeNumber1.value<TypeTag::NUMBER>())
              << std::endl;

    ValueTree treeNumber2(3.1415926);
    std::cout << "treeNumber2: state = " << int(treeNumber2.state())
              << ", value = " << (*treeNumber2.value<TypeTag::NUMBER>())
              << std::endl;

    ValueTree treeString("sadfasfsadfas");
    std::cout << "treeString: state = " << int(treeString.state())
              << ", value = " << (*treeString.value<TypeTag::STRING>())
              << std::endl;

    auto treeArray = ValueTree::from(std::vector{ 2, 3, 4, 5 });
    std::cout << "treeArray: state = " << int(treeArray.state())
              << ", value = " << json::dump(treeArray) << std::endl;

    auto treeObject = ValueTree::from(std::map<std::string, int>{
        { "1", 1 },
        { "2", 2 },
        { "3", 3 },
    });
    std::cout << "treeObject: state = " << int(treeObject.state())
              << ", value = " << json::dump(treeObject) << std::endl;

    ValueTree tree;
    tree["aaa"]["bbb"]["ccc"]["ddd"] = 9;
    tree["aaa"]["bbb"]["ddd"] = treeNumber1;
    tree["aaa"]["bbb"]["eee"] = treeNumber2;
    tree["aaa"]["bbb"]["fff"].asArray().emplace_back(NONE);
    auto& array = tree["aaa"]["bbb"]["fff"].asArray();
    array.emplace_back(treeArray);
    array.emplace_back(treeObject);
    auto& sensorConfig = tree["sensors"];
    sensorConfig["sensor1"]["name"] = "sensor1";
    sensorConfig["sensor1"]["type"] = "temperature";
    sensorConfig["sensor1"]["value"] = 26;
    sensorConfig["sensor2"]["name"] = "sensor2";
    sensorConfig["sensor2"]["type"] = "height";
    sensorConfig["sensor2"]["value"] = 42.0;
    std::cout << "tree: " << json::dump(tree, true, 4) << std::endl;

    std::cout << "tree[sensors][sensor1][name](Should be OK): "
              << tree.value<TypeTag::STRING>("sensors", "sensor1", "name")
                     .value_or("<Not-Found>")
              << std::endl;
    std::cout << "tree[sensors][sensor3][name](Key not exist): "
              << tree.value<TypeTag::STRING>("sensors", "sensor3", "name")
                     .value_or("<Not-Found>")
              << std::endl;
    std::cout << "tree[sensors][sensor1][name](Type not match): "
              << (tree.value<TypeTag::NUMBER>("sensors", "sensor1", "name")
                          == std::nullopt
                      ? "<Not-Found>"
                      : "<Found>")
              << std::endl;

    std::cout << "tree[aaa][bbb][fff][0](Should be OK): "
              << (tree.value<TypeTag::NONE>("aaa", "bbb", "fff", 0) == NONE
                      ? "NONE"
                      : "NOT NONE")
              << std::endl;
    std::cout << "tree[aaa][bbb][fff][1](Type not match): "
              << (tree.value<TypeTag::NONE>("aaa", "bbb", "fff", 1) == NONE
                      ? "NONE"
                      : "NOT NONE")
              << std::endl;

    auto subTree = tree.subTree("aaa", "bbb", "fff", 1);
    if (subTree) {
        std::cout << "Found subtree: tree[aaa][bbb][fff][1], state: "
                  << int(subTree->state()) << std::endl;
    }
}
