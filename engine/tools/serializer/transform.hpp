#pragma once

#include "engine/core/components/transform.hpp"
#include "engine/tools/serializer.hpp"

/* Deserialize only */
template <typename... Args>
inline void tag_invoke(JsonReflect::deserialize_t, const JsonReflect::json& j, tmt::Transform& value, Args&&... args) {
    JsonReflect::Detail::from_json_visitable(j, value, std::forward<Args>(args)...);
    value.mark_dirty();
    value.set_parent(value.get_parent());  // Re-apply to also update parent
}