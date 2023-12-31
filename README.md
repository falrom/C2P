# C2P

C++ 配置转参数工具。  
User configurations to program parameters.

```
[  Default Values   ] --\
                         \
[   CLI Arguments   ] ----> parse -> [Config] -> rules -> [Parameters]
                         /
[Configuration Files] --/
```

Parse user inputs (such as command line arguments or config files) to configuration struct, then generate program parameters.

The need to distinguish between "config" and "param" is because "config" usually needs to be defined in a form that is friendly and convenient for human understanding, while "param" needs to be closer to the input requirements of the program.

Redundant conversions between them should be avoided, and performance can be improved by directly defining "param" as a cache data type that can be used directly by the program.
