#pragma once

#include <JsonReflect.hpp>
#include <ImReflect_macro.hpp>

#include "engine/tools/serializer.hpp"

#define EXPAND(...) __VA_ARGS__

/* Used to reflect regular objects */
#define TMT_REFLECTION_IMPL(Type, ImguiFields, JsonFields) \
    IMGUI_REFLECT(Type, EXPAND ImguiFields);               \
    JSON_REFLECT(Type, EXPAND JsonFields)

#define TMT_OBJECT(Type, Fields) TMT_REFLECTION_IMPL(Type, Fields, Fields)

#define TMT_OBJECT_EX(Type, JsonFields, ImguiFields) TMT_REFLECTION_IMPL(Type, ImguiFields, JsonFields)

namespace tmt {
template <typename T>
struct Component {
    static constexpr const char* NAME = "Unknown";

    // Optional: helper function
    static constexpr const char* get_name() { return NAME; }
};
}  // namespace tmt

/* Used to reflect components */
#define TMT_COMPONENT_IMPL(Type, Name, ImguiFields, JsonFields)  \
    namespace tmt {                                              \
    template <>                                                  \
    struct Component<Type> {                                     \
        static constexpr const char* NAME = Name;                \
        static constexpr const char* get_name() { return NAME; } \
    };                                                           \
    }                                                            \
    TMT_REFLECTION_IMPL(Type, ImguiFields, JsonFields)

#define TMT_COMPONENT(Type, Name, Fields) TMT_COMPONENT_IMPL(Type, Name, Fields, Fields)

#define TMT_COMPONENT_EX(Type, Name, JsonFields, ImguiFields) TMT_COMPONENT_IMPL(Type, Name, ImguiFields, JsonFields)
