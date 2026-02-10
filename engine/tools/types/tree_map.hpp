#pragma once
#include <map>
#include <unordered_map>
#include <optional>
#include <stdexcept>
#include <iostream>

namespace tmt {

template <typename K, typename V, bool ORDERED = true>
class TreeMap {
   private:
    using MapType = std::conditional_t<ORDERED, std::map<K, TreeMap>, std::unordered_map<K, TreeMap>>;

    std::optional<V> value_;
    MapType children;

   public:
    // ============= CONSTRUCTORS =============
    TreeMap() : value_(std::nullopt), children() {}

    TreeMap(const V& val) : value_(val), children() {}

    TreeMap(const MapType& map) : value_(std::nullopt), children(map) {}

    TreeMap(const K& key, const V& val) : value_(std::nullopt), children {{key, TreeMap(val)}} {}

    // ============= CHILD ACCESS =============
    TreeMap& operator[](const K& key) { return children[key]; }

    TreeMap& get(const K& key) { return children[key]; }

    TreeMap& at(const K& key) { return children.at(key); }

    const TreeMap& at(const K& key) const { return children.at(key); }

    bool contains(const K& key) const { return children.contains(key); }

    // ============= VALUE MANAGEMENT =============
    TreeMap& operator=(const V& val) {
        value_ = val;
        return *this;
    }

    void set_value(const V& val) { value_ = val; }

    std::optional<V> get_value() const { return value_; }

    V& value() {
        if (!value_.has_value()) {
            throw std::runtime_error("Node does not have a value");
        }
        return *value_;
    }

    const V& value() const {
        if (!value_.has_value()) {
            throw std::runtime_error("Node does not have a value");
        }
        return *value_;
    }

    void clear_value() { value_ = std::nullopt; }

    bool has_value() const { return value_.has_value(); }

    // ============= NODE STATE QUERIES =============
    bool has_children() const { return !children.empty(); }

    size_t child_count() const { return children.size(); }

    // Legacy compatibility (can now both be true!)
    bool is_leaf() const { return value_.has_value(); }

    bool is_branch() const { return !children.empty(); }

    bool has_branches() const { return !children.empty(); }

    // ============= ITERATION =============
    auto begin() { return children.begin(); }
    auto end() { return children.end(); }
    auto begin() const { return children.begin(); }
    auto end() const { return children.end(); }

    // ============= UTILITIES =============
    void print(const std::string& prefix = "", const K& key = K {}, bool is_root = true) const {
        if (!is_root) {
            const std::string key_str = Serializer::serialize(key).dump();
            std::cout << prefix << "Key: " << key_str;
            if (value_.has_value()) {
                const std::string value_str = Serializer::serialize(*value_).dump();
                std::cout << " -> Value: " << value_str;
            }
            std::cout << '\n';
        }

        for (const auto& [child_key, child_node] : children) {
            child_node.print(prefix + "  ", child_key, false);
        }
    }
};

}  // namespace tmt