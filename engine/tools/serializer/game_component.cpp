#include "game_component.hpp"
#include <nlohmann/json.hpp>
#include "engine/systems/gameplay/game_component.hpp"

template <typename... Args>
JsonReflect::json tag_invoke(JsonReflect::serialize_lib_t, const tmt::IGameComponent& value, Args&&... args) {
    tmt::SerializationContext ctx;
    if constexpr (sizeof...(Args) > 0) {
        auto tuple = std::forward_as_tuple(args...);
        using FirstArgType = std::decay_t<decltype(std::get<0>(tuple))>;
        if constexpr (std::is_same_v<FirstArgType, tmt::SerializeState>) {
            ctx.set_serialize_state(&std::get<0>(tuple));
        }
    }
    return value.serialize(ctx);
}

template <typename... Args>
void tag_invoke(JsonReflect::deserialize_lib_t, const JsonReflect::json& j, tmt::IGameComponent& value, Args&&... args) {
    tmt::SerializationContext ctx;
    if constexpr (sizeof...(Args) > 0) {
        auto tuple = std::forward_as_tuple(args...);
        using FirstArgType = std::decay_t<decltype(std::get<0>(tuple))>;
        if constexpr (std::is_same_v<FirstArgType, tmt::DeserializeState>) {
            ctx.set_deserialize_state(&std::get<0>(tuple));
        }
    }
    value.deserialize(j, ctx);
}

/* forward declare */
struct SerializeState;
namespace tmt {
struct DeserializeState;
}

/* Explicit instantiations */
template JsonReflect::json tag_invoke<>(JsonReflect::serialize_lib_t, const tmt::IGameComponent&);
template JsonReflect::json tag_invoke<tmt::SerializeState&>(JsonReflect::serialize_lib_t, const tmt::IGameComponent&, tmt::SerializeState&);

template void tag_invoke<>(JsonReflect::deserialize_lib_t, const JsonReflect::json&, tmt::IGameComponent&);
template void tag_invoke<tmt::DeserializeState&>(JsonReflect::deserialize_lib_t, const JsonReflect::json&, tmt::IGameComponent&, tmt::DeserializeState&);