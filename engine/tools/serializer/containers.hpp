#include <set>
#include <map>
#include <unordered_set>

#include "engine/tools/serializer.hpp"

namespace tmt {

struct SerializeState;
struct DeserializeState;

}  // namespace tmt

// Specialization for std::set
template <typename T>
tmt::json tag_invoke(JsonReflect::serialize_t, const std::set<T>& set, tmt::SerializeState& state) {
    std::vector<tmt::json> elements;
    for (const auto& item : set) {
        elements.push_back(tmt::Serializer::serialize(item, state));
    }
    std::sort(elements.begin(), elements.end());
    return elements;
}

// Specialization for std::multiset
template <typename T>
tmt::json tag_invoke(JsonReflect::serialize_t, const std::multiset<T>& set, tmt::SerializeState& state) {
    std::vector<tmt::json> elements;
    for (const auto& item : set) {
        elements.push_back(tmt::Serializer::serialize(item, state));
    }
    std::sort(elements.begin(), elements.end());
    return elements;
}

// Specialization for std::map
template <typename K, typename V>
tmt::json tag_invoke(JsonReflect::serialize_t, const std::map<K, V>& map, tmt::SerializeState& state) {
    // Transform keys first, then sort
    std::vector<std::pair<tmt::json, tmt::json>> transformed;
    for (const auto& [key, value] : map) {
        transformed.emplace_back(tmt::Serializer::serialize(key, state), tmt::Serializer::serialize(value, state));
    }

    // Sort by transformed keys
    std::sort(transformed.begin(), transformed.end(), [](const auto& a, const auto& b) { return a.first < b.first; });

    // Build JSON object
    tmt::json result = tmt::json::object();
    for (const auto& [k, v] : transformed) {
        result[k.dump()] = v;  // nlohmann::json requires string keys
    }
    return result;
}

// Specialization for std::unordered_set (for consistent diffing)
template <typename T>
tmt::json tag_invoke(JsonReflect::serialize_t, const std::unordered_set<T>& set, tmt::SerializeState& state) {
    std::vector<tmt::json> elements;
    for (const auto& item : set) {
        elements.push_back(tmt::Serializer::serialize(item, state));
    }
    std::sort(elements.begin(), elements.end());  // Sort for deterministic output
    return elements;
}

// Specialization for std::unordered_map
template <typename K, typename V>
tmt::json tag_invoke(JsonReflect::serialize_t, const std::unordered_map<K, V>& map, tmt::SerializeState& state) {
    std::vector<std::pair<tmt::json, tmt::json>> transformed;
    for (const auto& [key, value] : map) {
        transformed.emplace_back(tmt::Serializer::serialize(key, state), tmt::Serializer::serialize(value, state));
    }

    std::sort(transformed.begin(), transformed.end(), [](const auto& a, const auto& b) { return a.first < b.first; });

    tmt::json result = tmt::json::object();
    for (const auto& [k, v] : transformed) {
        result[k.dump()] = v;
    }
    return result;
}