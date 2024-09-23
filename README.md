# C2P

C++ 配置转参数工具。  
User configurations to program parameters.

Parse user inputs (such as command line arguments or config files) to configuration struct, then generate program parameters.

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

We treat various user inputs as the source and parse them into a common intermediate [ValueTree](include/c2p/value_tree.hpp) (composed of multiple `ValueNode`s). After that, you can define the conversion from `ValueTree` to `Config` as well as override rules for different input methods.

Some tools are provided for parsing inputs, such as [JSON serialization/deserialization](include/c2p/json.hpp), command-line argument parsing (TODO), etc. All of these produce a `ValueTree` as the output.
