#pragma once

#include "engine/core/components/transform.hpp"
#include "engine/tools/serializer.hpp"

/* Deserialize only */
inline void tag_invoke(JsonReflect::deserialize_t, const JsonReflect::json& j, tmt::Transform& value) {
    JsonReflect::Detail::from_json_visitable(j, value);
    value.set_parent(value.get_parent());  // Re-apply to also update parent
    value.mark_dirty();
}