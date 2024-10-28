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
> Example: [example/example_value_tree.cpp](example/example_value_tree.cpp)

// TODO: Add more details about ValueTree.

### JSON

> API: [JSON serialization/deserialization](include/c2p/json.hpp)  
> Example: [example/example_json.cpp](example/example_json.cpp)

Parse JSON string into `ValueTree`, or serialize `ValueTree` into JSON string.

Extended JSON Grammar:
- Allow trailing comma in arrays and objects.
- Allow '+' sign for positive numbers.
- Allow single-line comment starts with '//'.

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
> Example: [example/example_ini.cpp](example/example_ini.cpp)

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

"" = value of empty key ; allow quoted string as empty key)
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
> Example: [example/example_cli.cpp](example/example_cli.cpp)

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
    .minPositionalArgNum = 2,
    .maxPositionalArgNum = 6,
};
const auto parser = c2p::cli::Parser::constructFrom(cg);
std::cout << (*(*parser).getHelp()) << std::endl;
```

And you will get the help string like this:

```shell
Usage:

  myprog [-v] [-h] <positionalArg0> <positionalArg1> [positionalArg2...5]

  Write the description of your program here.

Flag Arguments:

  -v, --version
    Show version information.

  -h, --help
    Show help information.

Positional Arguments:

  Need 2 ~ 6 positional argument(s).
```

There is a more complete example in [example/example_cli.cpp](example/example_cli.cpp).
