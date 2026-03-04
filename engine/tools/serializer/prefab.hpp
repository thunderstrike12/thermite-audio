#pragma once

#include "engine/core/components/prefab.hpp"
#include "engine/tools/serializer.hpp"

/* Deserialize only */
template <typename... Args>
inline void tag_invoke(JsonReflect::deserialize_t, const JsonReflect::json& j, tmt::Prefab& value, Args&&...) {
    /* Don't send args */
    JsonReflect::Detail::from_json_visitable(j, value);
}