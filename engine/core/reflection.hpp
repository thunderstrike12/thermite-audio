#pragma once

#define JSON_REFLECT_ENABLE_CONST_CAST
// #define JSON_REFLECT_INITIALIZE_SMART_POINTERS
#ifndef JSON_RELFECT_ALLOW_THROW
    #define JSON_RELFECT_ALLOW_THROW 0
#endif
#include <JsonReflect.hpp>
#include <ImReflect_macro.hpp>

#include "engine/tools/serializer.hpp"
#include "engine/tools/component_registry.hpp"

#define EXPAND(...) __VA_ARGS__

/* Used to reflect regular objects */
#define TMT_REFLECTION_IMPL(Type, JsonFields, ImguiFields) \
    JSON_REFLECT(Type, EXPAND JsonFields);                 \
    IMGUI_REFLECT(Type, EXPAND ImguiFields)

#define TMT_OBJECT(Type, Fields) TMT_REFLECTION_IMPL(Type, Fields, Fields)

#define TMT_OBJECT_EX(Type, JsonFields, ImguiFields) TMT_REFLECTION_IMPL(Type, JsonFields, ImguiFields)

#define TMT_OBJECT_SERIALIZE(Type, Fields) JSON_REFLECT(Type, Fields)

#define TMT_OBJECT_SERIALIZE_TEMPLATE(TPARAMS, Type, TARGS, ...) JSON_REFLECT_TEMPLATE(TPARAMS, Type, TARGS, __VA_ARGS__)

#define TMT_OBJECT_SERIALIZE_EMPTY(Type)                                                  \
    inline tmt::json tag_invoke(JsonReflect::serialize_t, const Type&) {                  \
        return nlohmann::json::object();                                                  \
    }                                                                                     \
    inline void tag_invoke(JsonReflect::deserialize_t, const JsonReflect::json&, Type&) { \
        /* No fields to deserialize */                                                    \
    }                                                                                     \
    static_assert(true, "")

#define TMT_OBJECT_INSPECT(Type, Fields) IMGUI_REFLECT(Type, Fields)

#define TMT_OBJECT_INSPECT_TEMPLATE(TPARAMS, Type, TARGS, ...) IMGUI_REFLECT_TEMPLATE(TPARAMS, Type, TARGS, __VA_ARGS__)

#define TMT_OBJECT_INSPECT_EMPTY(Type)                                                           \
    inline void tag_invoke(ImReflect::ImInput_t, const char*, Type&, ImSettings&, ImResponse&) { \
        /* No fields to inspect */                                                               \
    }                                                                                            \
    static_assert(true, "")

#define TMT_OBJECT_TEMPLATE(TPARAMS, Type, TARGS, ...)                \
    TMT_OBJECT_SERIALIZE_TEMPLATE(TPARAMS, Type, TARGS, __VA_ARGS__); \
    TMT_OBJECT_INSPECT_TEMPLATE(TPARAMS, Type, TARGS, __VA_ARGS__)

#define TMT_OBJECT_EMPTY(Type)        \
    TMT_OBJECT_SERIALIZE_EMPTY(Type); \
    TMT_OBJECT_INSPECT_EMPTY(Type)

namespace tmt {

template <typename T>
struct Component {
    static constexpr const char* NAME = "Unknown";

    // Optional: helper function
    static constexpr const char* get_name() { return NAME; }
};

}  // namespace tmt

#define TMT_COMPONENT_NAME(Type, Name)                           \
    namespace tmt {                                              \
                                                                 \
    template <>                                                  \
    struct Component<Type> {                                     \
        static constexpr const char* NAME = Name;                \
        static constexpr const char* get_name() { return NAME; } \
    };                                                           \
                                                                 \
    }

/* Used to reflect components */
#define TMT_COMPONENT_IMPL(Type, Name, ImguiFields, JsonFields) \
    TMT_COMPONENT_NAME(Type, Name);                             \
    TMT_REFLECTION_IMPL(Type, ImguiFields, JsonFields);

#define TMT_COMPONENT(Type, Name, Fields) TMT_COMPONENT_IMPL(Type, Name, Fields, Fields)

#define TMT_COMPONENT_EX(Type, Name, JsonFields, ImguiFields) TMT_COMPONENT_IMPL(Type, Name, ImguiFields, JsonFields)

#define TMT_COMPONENT_SERIALIZE(Type, Fields) JSON_REFLECT(Type, EXPAND Fields)

#define TMT_COMPONENT_SERIALIZE_EMPTY(Type) TMT_OBJECT_SERIALIZE_EMPTY(Type)

#define TMT_COMPONENT_INSPECT(Type, Fields) IMGUI_REFLECT(Type, EXPAND Fields)

#define TMT_COMPONENT_INSPECT_EMPTY(Type) TMT_OBJECT_INSPECT_EMPTY(Type)

namespace tmt {

template <typename T>
struct ComponentDependencies {
    using Dependencies = ComponentRegistry<>;
};

}  // namespace tmt

#define TMT_COMPONENT_DEPENDENCIES(Type, ...)                \
    namespace tmt {                                          \
                                                             \
    template <>                                              \
    struct ComponentDependencies<Type> {                     \
        using Dependencies = ComponentRegistry<__VA_ARGS__>; \
    };                                                       \
                                                             \
    }                                                        \
    static_assert(true, "")