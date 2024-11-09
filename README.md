# C2P

C++ 配置转参数工具包。  
User configurations to program parameters tools.

Parse user inputs (e.g. [CLI arguments](#cli), [JSON](#json), [INI](#ini), etc.) to configuration struct, then generate program parameters.

```
[DefaultValues] --parse-> [ValueTree] \                    [Rules]
                                       \                      |
[ ConfigFiles ] --parse-> [ValueTree] --join-> [Config] --transform-> [Param]
                                       /
[CLI-Arguments] --parse-> [ValueTree] /
```

## Transform from Config to Param with Rules

Generally, `Config` should be defined in a human-friendly format, while `Param` needs to be closer to the program's input requirements. Defining `Param` as a cached data type that the program can directly work with (avoiding redundant checks and type conversions) also helps improve performance. The bridge that connects the two is the `Rule`s.

Define your own `Config` and `Param` types separately. Then, create a set of `Rule`s to:

- Handle part of the conversion from `Config` to `Param`.
- Perform input validation and report errors if requirements are not met.

Finally, call the transform function to apply these `Rule`s.

## Parsing Config from IO

We treat various user inputs as the source and parse them into a common intermediate [ValueTree](#valuetree) (composed of multiple `ValueNode`s). After that, you can define the conversion from `ValueTree` to `Config` as well as override rules for different input methods.

Some tools are provided for parsing inputs, such as [CLI arguments](#cli), [JSON](#json), [INI](#ini), etc. All of these produce a `ValueTree` as the output.

### ValueTree

> API: [ValueTree](include/c2p/value_tree.hpp)  
> Example: [examples/example_value_tree.cpp](examples/example_value_tree.cpp)

A `ValueTree` represents a tree-structured data, which can recursively contain multiple child `ValueTree`s.

Each `ValueTree` has 4 possible states:

- `EMPTY`: Empty state, representing an empty tree with no child nodes or values. Can be implicitly recognized as `false`.
- `VALUE`: Value state, representing a leaf node with a value.
- `ARRAY`: Array state, representing an array node with multiple child `ValueTree`s.
- `OBJECT`: Object state, representing an object node with multiple child `ValueTree`s, each child node has a corresponding key.

A leaf node must be a value, wrapped by a `ValueNode` object. The value type of `ValueNode` can be one of the 4 types: `NONE`, `BOOL`, `NUMBER`, `STRING`.

The design of the `ValueTree` class refers to the structure of JSON. For example, you can construct a `ValueTree` in the following way:

```cpp
using namespace c2p;

ValueTree tree;

// normal root level key-value pair
tree["name"] = "Alice";
tree["age"] = 20;
tree["job"] = NONE;

// object (method 1)
tree["info"]["phone"] = "123-456-7890";
tree["info"]["email"] = "alice@fake.com";

// object (method 2)
auto& address = tree["info"]["address"].asObject();
address["contry"] = "USA";
address["city"] = "New York";
address["zip"] = 10001;

// array (method 1)
tree["family"] = ValueTree::from(std::vector{
    "Grandpa",
    "Grandma",
    "Dad",
    "Mom",
});

// array (method 2)
auto& roommates = tree["roommates"].asArray();
roommates = {
    ValueNode("Bob"),
    ValueNode("Charlie"),
    ValueNode("David"),
};
roommates.emplace_back("Eve");
roommates.emplace_back("Frank");
```

Equivalent JSON:
```json
{
    "name": "Alice",
    "age": 20,
    "job": null,
    "info": {
        "phone": "123-456-7890",
        "email": "alice@fake.com",
        "address": {
            "contry": "USA",
            "city": "New York",
            "zip": 10001
        }
    },
    "family": ["Grandpa", "Grandma", "Dad", "Mom"],
    "roommates": ["Bob", "Charlie", "David", "Eve", "Frank"]
}
```

To read a const `ValueTree`, you can use the following methods:

```cpp
// Assume you have a const ValueTree object
const ValueTree& constTree = tree;

// specify the key path
const auto phone = constTree.value<TypeTag::STRING>("info", "phone");
if (phone) {
    std::cout << "Phone: " << *phone << std::endl;
}

// the index of the array is also supported
const auto bestFriend = constTree.value<TypeTag::STRING>("roommates", 2);
if (bestFriend) {
    std::cout << "Best friend: " << *bestFriend << std::endl;
}

// if you already get a leaf node, you can use `value()` directly
const auto leaf = constTree.subTree("name");
assert(leaf);
assert(leaf->state() == ValueTree::State::VALUE);
const auto name = leaf->value<TypeTag::STRING>();
if (name) {
    std::cout << "Name: " << *name << std::endl;
}

// Output:
// Phone: 123-456-7890
// Best friend: David
// Name: Alice
```

For more information, please refer to the example: [examples/example_value_tree.cpp](examples/example_value_tree.cpp)

### JSON

> API: [JSON serialization/deserialization](include/c2p/json.hpp)  
> Example: [examples/example_json.cpp](examples/example_json.cpp)

Parse JSON string into `ValueTree`, or serialize `ValueTree` into JSON string.

Extended JSON Grammar:
- Allow trailing comma in arrays and objects.
- Allow '+' sign for positive numbers.
- Allow single-line comment starts with "//".

Example:

```json
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
```

### INI

> API: [INI serialization/deserialization](include/c2p/ini.hpp)  
> Example: [examples/example_ini.cpp](examples/example_ini.cpp)

Parse INI string into `ValueTree`, or serialize `ValueTree` into INI string.

Extended INI Grammar:
- Allow single-line comments starting with ';' or '#'.
- Allow no-section key-value pairs at the beginning.
- Allow empty value string even without quotes.

Example:

```ini
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

"" = value of empty key ; allow quoted string as empty key
```

Equivalent JSON:
```json
{
    "name": "John Doe",
    "age": "30",
    "city": "New York",
    "section 1": {
        "email": "name@fake.com",
        "home addr": "银河系 - 太阳系 - 地球: 北极点"
    },
    "": {
        "empty info": "",
        "empty info2": "",
        "": "value of empty key"
    }
}
```

### CLI

> API: [command-line argument parsing](include/c2p/cli.hpp)  
> Example: [examples/example_cli.cpp](examples/example_cli.cpp)

Parse CLI arguments into `ValueTree`.

Support recursive nested subcommands, and three types of arguments can be used:

- flag arguments
- value arguments
- positional arguments

You need to define a `CommandGroup` first, which can help you construct a CLI parser. The construction process will check whether the configuration you defined is legal.  
If the parser is constructed successfully, you can call the `parse` method of the parser to parse the arguments. You can also call the `getHelp` method to automatically generate a help string for interactive printing.

Example:

```cpp
c2p::cli::CommandGroup cg = {
    .command = "myprog",
    .description = "Write the description of your program here.",
    .flagArgs = {
        { .name = "version", .shortName = 'v', .description = "Show version information." },
        { .name = "help",    .shortName = 'h', .description = "Show help information."    },
    },
    .valueArgs = {
        {
            .name = "width",
            .shortName = 'w',
            .typeTag = c2p::TypeTag::NUMBER,
            .required = true,
            .description = "Specify the width of the image."
        },
        {
            .name = "height",
            .typeTag = c2p::TypeTag::NUMBER,
            .description = "Specify the height of the image."
        },
        {
            .name = "output",
            .shortName = 'o',
            .typeTag = c2p::TypeTag::STRING,
            .description = "Specify output file path."
        },
    },
    .minPositionalArgNum = 2,
    .maxPositionalArgNum = 6,
};
const auto parser = c2p::cli::Parser::constructFrom(cg);
std::cout << (*(*parser).getHelp()) << std::endl;
```

And you will get the help string like this:

```shell
Usage:

  myprog -w <NUMBER> [--height <NUMBER>] [-o <STRING>] [-v] [-h] <positionalArg0> <positionalArg1> [positionalArg2...5]

  Write the description of your program here.

Flag Arguments:

  -v, --version
    Show version information.

  -h, --help
    Show help information.

Required Value Arguments:

  -w, --width <NUMBER>
    Specify the width of the image.

Optional Value Arguments:

  --height <NUMBER>
    Specify the height of the image.

  -o, --output <STRING>
    Specify output file path.

Positional Arguments:

  Need 2 ~ 6 positional argument(s).
```

Assume you have the following CLI arguments:

```shell
myprog -v -w 1920 -h -o output.png input1.jpg input2.jpg
```

You can parse them into a `ValueTree` like this (print as JSON):

```JSON
{
    "command": "myprog",
    "flagArgs": [
        "version",
        "help"
    ],
    "positionalArgs": [
        "input1.jpg",
        "input2.jpg"
    ],
    "valueArgs": {
        "output": "output.png",
        "width": 1920
    }
}
```

There is a more complete example in [examples/example_cli.cpp](examples/example_cli.cpp).
