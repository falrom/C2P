/**
 * @file value_tree.hpp
 * @author Common configuration value tree definition.
 */

#ifndef __C2P_VALUE_TREE_HPP__
#define __C2P_VALUE_TREE_HPP__

#include <cstddef>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace c2p {
namespace value_tree {

class ValueNode;

struct NoneValue {
    constexpr explicit NoneValue(int) {}
    bool operator==(const NoneValue&) const { return true; }
    bool operator!=(const NoneValue&) const { return false; }
};
constexpr NoneValue NONE(0);

using BoolValue = bool;
using NumberValue = double;
using StringValue = std::string;
using ArrayValue = std::vector<ValueNode>;
using ObjectValue = std::map<std::string, ValueNode>;

// clang-format off
/// The number and order of template types must be consistent with enum `ValueType`.
using Value = std::variant< NoneValue, BoolValue, NumberValue, StringValue, ArrayValue, ObjectValue >;
enum class ValueType      { NONE,      BOOL,      NUMBER,      STRING,      ARRAY,      OBJECT      };
// clang-format on

using ValuePtr = std::shared_ptr<Value>;
using ConstValuePtr = std::shared_ptr<const Value>;

template <ValueType typeEnum>
struct TypeOfEnum {
    using type = std::variant_alternative_t<size_t(typeEnum), Value>;
};

/// Definition of value tree node. A node and its descendants can represent a
/// tree structure. It can be used as an intermediate data structure for parsing
/// and generating configuration files, JSON, etc.
class ValueNode
{
  public:

    /// If value is NONE, return false.
    operator bool() const { return type() != ValueType::NONE; }

  public:

    /// Get type of stored value.
    ValueType type() const { return static_cast<ValueType>(_value.index()); }

    /// Try to get stored value.
    /// If current value is NOT ( NONE | BOOL | NUMBER | STRING ), return
    /// std::nullopt. If current value is NOT the same as template type, return
    /// std::nullopt.
    template <ValueType typeEnum>
    auto value() const -> std::optional<typename TypeOfEnum<typeEnum>::type> {
        if constexpr (typeEnum == ValueType::NONE || typeEnum == ValueType::BOOL
                      || typeEnum == ValueType::NUMBER
                      || typeEnum == ValueType::STRING)
        {
            if (type() != typeEnum) return std::nullopt;
            else return std::get<size_t(typeEnum)>(_value);
        } else return std::nullopt;
    }

    /// Get ArrayValue reference.
    /// If current value is NOT an array, change it to an empty array.
    ArrayValue& asArray() {
        if (type() != ValueType::ARRAY) {
            _value = ArrayValue();
        }
        return std::get<size_t(ValueType::ARRAY)>(_value);
    }

    /// Get ObjectValue reference.
    /// If current value is NOT an object, change it to an empty object.
    ObjectValue& asObject() {
        if (type() != ValueType::OBJECT) {
            _value = ObjectValue();
        }
        return std::get<size_t(ValueType::OBJECT)>(_value);
    }

    /// Get stored value reference at ObjectValue[key].
    /// If current value is NOT an object, change it to NONE object.
    /// If key does not exist, create and set as NONE.
    ValueNode& operator[](const std::string& key) {
        if (type() != ValueType::OBJECT) {
            _value = ObjectValue();
        }
        return std::get<size_t(ValueType::OBJECT)>(_value)[key];
    }

    /// Get stored value reference at ObjectValue[key].
    /// If current value is NOT an object, change it to NONE object.
    /// If key does not exist, create and set as NONE.
    ValueNode& operator[](const char* key) {
        if (type() != ValueType::OBJECT) {
            _value = ObjectValue();
        }
        return std::get<size_t(ValueType::OBJECT)>(_value)[key];
    }

  public:

    /// Default constructor. As NONE.
    ValueNode() = default;

    /// For NONE.
    ValueNode(NoneValue): _value(NONE) {}

    /// For const char*. Cast to std::string.
    ValueNode(const char* value): _value(std::string(value)) {}

    /// For std::string_view. Cast to std::string.
    ValueNode(std::string_view value): _value(std::string(value)) {}

    /// For const std::string&.
    ValueNode(const std::string& value): _value(value) {}

    /// For std::string&&.
    ValueNode(std::string&& value): _value(value) {}

    /// For integral types. Cast to double.
    template <
        typename T,
        typename =
            std::enable_if_t<std::is_integral_v<T> || std::is_reference_v<T>>>
    ValueNode(T value): _value(double(value)) {}

    /// Automatically call the constructor of std::variant in other cases.
    template <
        typename T,
        typename =
            std::enable_if_t<!std::is_integral_v<T> && !std::is_reference_v<T>>>
    ValueNode(T&& value): _value(value) {}

    /// From std::vector to ArrayValue.
    template <typename T>
    static ValueNode from(std::vector<T>&& vector) {
        ArrayValue array;
        for (auto& value: vector) {
            array.push_back(ValueNode(value));
        }
        return ValueNode(std::move(array));
    }

    /// From std::map to ObjectValue.
    template <typename T>
    static ValueNode from(std::map<std::string, T>&& map) {
        ObjectValue object;
        for (auto& [key, value]: map) {
            object[key] = ValueNode(value);
        }
        return ValueNode(std::move(object));
    }

    ValueNode(const ValueNode&) = default;
    ValueNode(ValueNode&&) = default;
    ValueNode& operator=(const ValueNode&) = default;
    ValueNode& operator=(ValueNode&&) = default;

    ~ValueNode() = default;

  private:

    /// Store value here.
    Value _value = NONE;
};

}  // namespace value_tree
}  // namespace c2p

#endif  // __C2P_VALUE_TREE_HPP__
