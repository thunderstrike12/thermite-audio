#pragma once
#define JSON_REFLECT_ENABLE_CONST_CAST
// #define JSON_REFLECT_INITIALIZE_SMART_POINTERS
#ifndef JSON_RELFECT_ALLOW_THROW
    #define JSON_RELFECT_ALLOW_THROW 0
#endif
#include <JsonReflect.hpp>
#include "engine/tools/json.hpp"

#define BEFRIEND_VISITABLE()      \
    template <typename, typename> \
    friend struct ::visit_struct::traits::visitable;

namespace tmt {

class Serializer {
   public:
    template <typename T, typename... Args>
    static nlohmann::ordered_json serialize(const T& obj, Args&&... args) {
        return JsonReflect::to_json(obj, std::forward<Args>(args)...);
    }
    template <typename T, typename... Args>
    static void deserialize(const json& j, T& value, Args&&... args) {
        JsonReflect::from_json(j, value, std::forward<Args>(args)...);
    }

    struct Config {
        constexpr static uint64_t VERSION = 1;
    };
};

}  // namespace tmt
