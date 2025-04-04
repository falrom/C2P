/**
 * @file value_tree.hpp
 * @author Common configuration ValueTree definition.
 */

#ifndef __C2P_VALUE_TREE_HPP__
#define __C2P_VALUE_TREE_HPP__

#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace c2p {

struct NoneValue {
    enum class _Construct { _TOKEN };
    explicit constexpr NoneValue(_Construct) noexcept {}
    constexpr bool operator==(const NoneValue&) const { return true; }
    constexpr bool operator!=(const NoneValue&) const { return false; }
};
constexpr NoneValue NONE{ NoneValue::_Construct::_TOKEN };

using BoolValue = bool;
using NumberValue = double;
using StringValue = std::string;

// clang-format off
/// The number and order of template types must be consistent with enum `TypeTag`.
using Value = std::variant< NoneValue, BoolValue, NumberValue, StringValue >;
enum class TypeTag        { NONE,      BOOL,      NUMBER,      STRING,     };
// clang-format on

/// Get the corresponding value type through the TypeTag enumeration value.
template <TypeTag typeTag>
struct TypeOfTag {
    using type = std::variant_alternative_t<size_t(typeTag), Value>;
};

/// Convert TypeTag to string.
inline std::string to_string(TypeTag typeTag) {
    switch (typeTag) {
        case TypeTag::NONE: return "NONE";
        case TypeTag::BOOL: return "BOOL";
        case TypeTag::NUMBER: return "NUMBER";
        case TypeTag::STRING: return "STRING";
        default: return "UNKNOWN";
    }
}

/// Definition of `ValueTree` node which stores a value of an indeterminate
/// type. A `ValueTree` is organized by `ArrayNode` and `ObjectNode`. The
/// leaves of the `ValueTree` are all of `ValueNode` type, storing the actual
/// values.
class ValueNode
{
  public:

    /// Get TypeTag of stored value.
    TypeTag typeTag() const { return static_cast<TypeTag>(_value.index()); }

    /// If the stored value is NONE.
    bool isNone() const { return typeTag() == TypeTag::NONE; }

    /// If the stored value is BOOL.
    bool isBool() const { return typeTag() == TypeTag::BOOL; }

    /// If the stored value is NUMBER.
    bool isNumber() const { return typeTag() == TypeTag::NUMBER; }

    /// If the stored value is STRING.
    bool isString() const { return typeTag() == TypeTag::STRING; }

    /// Try to get stored value.
    /// If current value is NOT the same as template TypeTag,
    /// return std::nullopt.
    template <TypeTag tag>
    auto value() const -> std::optional<typename TypeOfTag<tag>::type> {
        if (typeTag() != tag) return std::nullopt;
        else return std::get<size_t(tag)>(_value);
    }

  public:

    /// Default constructor. As NONE.
    ValueNode() = default;

    /// For NONE.
    ValueNode(NoneValue): _value(NONE) {}

    /// For bool.
    ValueNode(bool value): _value(value) {}

    /// For const char*. Cast to std::string.
    ValueNode(const char* value): _value(std::string(value)) {}

    /// For std::string_view. Cast to std::string.
    ValueNode(std::string_view value): _value(std::string(value)) {}

    /// For const std::string&.
    ValueNode(const std::string& value): _value(value) {}

    /// For std::string&&.
    ValueNode(std::string&& value): _value(std::move(value)) {}

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
    ValueNode(T&& value): _value(std::move(value)) {}

    ValueNode(const ValueNode&) = default;
    ValueNode(ValueNode&&) = default;
    ValueNode& operator=(const ValueNode&) = default;
    ValueNode& operator=(ValueNode&&) = default;

    ~ValueNode() = default;

  private:

    /// Store value here.
    Value _value = NONE;
};

class ValueTree;

using ArrayNode = std::vector<ValueTree>;
using ObjectNode = std::map<std::string, ValueTree>;

/// Definition of `ValueTree`.
class ValueTree
{
  public:

    enum class State { EMPTY, VALUE, ARRAY, OBJECT };

    /// Get the state of the ValueTree root.
    State state() const { return _state; }

    /// Return false if is an empty tree.
    operator bool() const { return state() != State::EMPTY; }

    /// If the tree state is State::EMPTY.
    bool isEmpty() const { return state() == State::EMPTY; }

    /// If the tree state is State::VALUE.
    bool isValue() const { return state() == State::VALUE; }

    /// If the tree state is State::ARRAY.
    bool isArray() const { return state() == State::ARRAY; }

    /// If the tree state is State::OBJECT.
    bool isObject() const { return state() == State::OBJECT; }

  public:

    /// Clear the tree to an empty state.
    void clear() {
        _state = State::EMPTY;
        _value_node = NONE;
        _array_node.clear();
        _object_node.clear();
    }

    /// Get ValueNode reference.
    /// If current tree root is NOT a value, change it to ValueNode(NONE).
    ValueNode& asValue() {
        if (state() != State::VALUE) {
            clear();
            _state = State::VALUE;
        }
        return _value_node;
    }

    /// Get ArrayNode reference.
    /// If current tree root is NOT an array, change it to an empty array.
    ArrayNode& asArray() {
        if (state() != State::ARRAY) {
            clear();
            _state = State::ARRAY;
        }
        return _array_node;
    }

    /// Get ObjectNode reference.
    /// If current tree root is NOT an object, change it to an empty object.
    ObjectNode& asObject() {
        if (state() != State::OBJECT) {
            clear();
            _state = State::OBJECT;
        }
        return _object_node;
    }

    /// Get subtree reference at specified key.
    /// If current tree root is NOT an object, change it to an object, and
    /// create a new subtree at specified key.
    ValueTree& operator[](const std::string& key) { return asObject()[key]; }

    /// Get subtree reference at specified key.
    /// If current tree root is NOT an object, change it to an object, and
    /// create a new subtree at specified key.
    ValueTree& operator[](const char* key) { return asObject()[key]; }

  public:

    /// Try to get sub tree (pointer) at specified key.
    /// If state of current tree is NOT State::OBJECT, return nullptr.
    /// If key NOT found, return nullptr.
    ValueTree* subTree(const std::string& key) {
        if (state() != State::OBJECT) return nullptr;
        const auto it = _object_node.find(key);
        if (it == _object_node.end()) return nullptr;
        return &(it->second);
    }

    /// Try to get sub tree (pointer) at specified key.
    /// If state of current tree is NOT State::OBJECT, return nullptr.
    /// If key NOT found, return nullptr.
    const ValueTree* subTree(const std::string& key) const {
        if (state() != State::OBJECT) return nullptr;
        const auto it = _object_node.find(key);
        if (it == _object_node.end()) return nullptr;
        return &(it->second);
    }

    /// Try to get sub tree (pointer) at specified path.
    /// If state of current tree is NOT State::OBJECT, return nullptr.
    /// If path NOT found, return nullptr.
    template <typename... Args>
    ValueTree* subTree(const std::string& key, Args&&... args) {
        if (state() != State::OBJECT) return nullptr;
        const auto it = _object_node.find(key);
        if (it == _object_node.end()) return nullptr;
        return it->second.subTree(std::forward<Args>(args)...);
    }

    /// Try to get sub tree (pointer) at specified path.
    /// If state of current tree is NOT State::OBJECT, return nullptr.
    /// If path NOT found, return nullptr.
    template <typename... Args>
    const ValueTree* subTree(const std::string& key, Args&&... args) const {
        if (state() != State::OBJECT) return nullptr;
        const auto it = _object_node.find(key);
        if (it == _object_node.end()) return nullptr;
        return it->second.subTree(std::forward<Args>(args)...);
    }

    /// Try to get sub tree (pointer) at specified index.
    /// If state of current tree is NOT State::ARRAY, return nullptr.
    /// If index NOT found, return nullptr.
    ValueTree* subTree(size_t index) {
        if (state() != State::ARRAY) return nullptr;
        if (index >= _array_node.size()) return nullptr;
        return &(_array_node[index]);
    }

    /// Try to get sub tree (pointer) at specified index.
    /// If state of current tree is NOT State::ARRAY, return nullptr.
    /// If index NOT found, return nullptr.
    const ValueTree* subTree(size_t index) const {
        if (state() != State::ARRAY) return nullptr;
        if (index >= _array_node.size()) return nullptr;
        return &(_array_node[index]);
    }

    /// Try to get sub tree (pointer) at specified path.
    /// If state of current tree is NOT State::ARRAY, return nullptr.
    /// If path NOT found, return nullptr.
    template <typename... Args>
    ValueTree* subTree(size_t index, Args&&... args) {
        if (state() != State::ARRAY) return nullptr;
        if (index >= _array_node.size()) return nullptr;
        return _array_node[index].subTree(std::forward<Args>(args)...);
    }

    /// Try to get sub tree (pointer) at specified path.
    /// If state of current tree is NOT State::ARRAY, return nullptr.
    /// If path NOT found, return nullptr.
    template <typename... Args>
    const ValueTree* subTree(size_t index, Args&&... args) const {
        if (state() != State::ARRAY) return nullptr;
        if (index >= _array_node.size()) return nullptr;
        return _array_node[index].subTree(std::forward<Args>(args)...);
    }

  public:

    /// Try to get ValueNode pointer.
    /// If state of current tree is NOT State::VALUE, return nullptr.
    ValueNode* getValue() {
        return (state() == State::VALUE) ? &_value_node : nullptr;
    }

    /// Try to get ValueNode pointer.
    /// If state of current tree is NOT State::VALUE, return nullptr.
    const ValueNode* getValue() const {
        return (state() == State::VALUE) ? &_value_node : nullptr;
    }

    /// Try to get ValueNode pointer at specified path.
    /// If path NOT found, return nullptr.
    /// If state of found tree is NOT State::VALUE, return nullptr.
    /// return nullptr.
    template <typename... Args>
    ValueNode* getValue(Args&&... args) {
        auto tree = subTree(std::forward<Args>(args)...);
        if (!tree) return nullptr;
        return tree->getValue();
    }

    /// Try to get ValueNode pointer at specified path.
    /// If path NOT found, return nullptr.
    /// If state of found tree is NOT State::VALUE, return nullptr.
    /// return nullptr.
    template <typename... Args>
    const ValueNode* getValue(Args&&... args) const {
        auto tree = subTree(std::forward<Args>(args)...);
        if (!tree) return nullptr;
        return tree->getValue();
    }

    /// Try to get ArrayNode pointer.
    /// If state of current tree is NOT State::ARRAY, return nullptr.
    ArrayNode* getArray() {
        return (state() == State::ARRAY) ? &_array_node : nullptr;
    }

    /// Try to get ArrayNode pointer.
    /// If state of current tree is NOT State::ARRAY, return nullptr.
    const ArrayNode* getArray() const {
        return (state() == State::ARRAY) ? &_array_node : nullptr;
    }

    /// Try to get ArrayNode pointer at specified path.
    /// If path NOT found, return nullptr.
    /// If state of found tree is NOT State::ARRAY, return nullptr.
    /// return nullptr.
    template <typename... Args>
    ArrayNode* getArray(Args&&... args) {
        auto tree = subTree(std::forward<Args>(args)...);
        if (!tree) return nullptr;
        return tree->getArray();
    }

    /// Try to get ArrayNode pointer at specified path.
    /// If path NOT found, return nullptr.
    /// If state of found tree is NOT State::ARRAY, return nullptr.
    /// return nullptr.
    template <typename... Args>
    const ArrayNode* getArray(Args&&... args) const {
        auto tree = subTree(std::forward<Args>(args)...);
        if (!tree) return nullptr;
        return tree->getArray();
    }

    /// Try to get ObjectNode pointer.
    /// If state of current tree is NOT State::OBJECT, return nullptr.
    ObjectNode* getObject() {
        return (state() == State::OBJECT) ? &_object_node : nullptr;
    }

    /// Try to get ObjectNode pointer.
    /// If state of current tree is NOT State::OBJECT, return nullptr.
    const ObjectNode* getObject() const {
        return (state() == State::OBJECT) ? &_object_node : nullptr;
    }

    /// Try to get ObjectNode pointer at specified path.
    /// If path NOT found, return nullptr.
    /// If state of found tree is NOT State::OBJECT, return nullptr.
    /// return nullptr.
    template <typename... Args>
    ObjectNode* getObject(Args&&... args) {
        auto tree = subTree(std::forward<Args>(args)...);
        if (!tree) return nullptr;
        return tree->getObject();
    }

    /// Try to get ObjectNode pointer at specified path.
    /// If path NOT found, return nullptr.
    /// If state of found tree is NOT State::OBJECT, return nullptr.
    /// return nullptr.
    template <typename... Args>
    const ObjectNode* getObject(Args&&... args) const {
        auto tree = subTree(std::forward<Args>(args)...);
        if (!tree) return nullptr;
        return tree->getObject();
    }

  public:

    /// Try to get stored value.
    /// If state of current tree is NOT State::VALUE, return std::nullopt.
    /// If value of current node is NOT the same as template TypeTag,
    /// return std::nullopt.
    template <TypeTag typeTag>
    auto value() const -> std::optional<typename TypeOfTag<typeTag>::type> {
        if (state() != State::VALUE) return std::nullopt;
        return _value_node.value<typeTag>();
    }

    /// Try to get stored value at specified path.
    /// If path NOT found, return std::nullopt.
    /// If state of found tree is NOT State::VALUE, return std::nullopt.
    /// If value of found node is NOT the same as template TypeTag,
    /// return std::nullopt.
    template <TypeTag typeTag, typename... Args>
    auto value(Args&&... args) const
        -> std::optional<typename TypeOfTag<typeTag>::type> {
        auto tree = subTree(std::forward<Args>(args)...);
        if (!tree) return std::nullopt;
        return tree->template value<typeTag>();
    }

  public:

    ~ValueTree() { clear(); }

    /// Default constructor. As an empty tree.
    ValueTree() = default;

    ValueTree(const ValueNode& node): _state(State::VALUE), _value_node(node) {}
    ValueTree(ValueNode&& node)
        : _state(State::VALUE), _value_node(std::move(node)) {}
    ValueTree& operator=(const ValueNode& node) {
        asValue() = node;
        return *this;
    }
    ValueTree& operator=(ValueNode&& node) {
        asValue() = std::move(node);
        return *this;
    }

    ValueTree(const ValueTree&) = default;
    ValueTree(ValueTree&&) = default;
    ValueTree& operator=(const ValueTree&) = default;
    ValueTree& operator=(ValueTree&&) = default;

    /// From std::vector to ValueTree with State::ARRAY.
    template <typename T>
    static ValueTree from(std::vector<T>&& vector) {
        ValueTree tree;
        auto& array = tree.asArray();
        for (auto& value: vector) {
            array.push_back(ValueTree(value));
        }
        return tree;
    }

    /// From std::map to ValueTree with State::OBJECT.
    template <typename T>
    static ValueTree from(std::map<std::string, T>&& map) {
        ValueTree tree;
        auto& object = tree.asObject();
        for (auto& [key, value]: map) {
            object[key] = ValueTree(value);
        }
        return tree;
    }

  private:

    State _state = State::EMPTY;

    ValueNode _value_node = NONE;
    ArrayNode _array_node = {};
    ObjectNode _object_node = {};
};

/// Convert ValueTree::State to string.
inline std::string to_string(ValueTree::State state) {
    switch (state) {
        case ValueTree::State::EMPTY: return "EMPTY";
        case ValueTree::State::VALUE: return "VALUE";
        case ValueTree::State::ARRAY: return "ARRAY";
        case ValueTree::State::OBJECT: return "OBJECT";
        default: return "UNKNOWN";
    }
}

}  // namespace c2p

#endif  // __C2P_VALUE_TREE_HPP__
