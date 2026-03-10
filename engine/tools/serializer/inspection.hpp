#pragma once
#include <ImReflect_macro.hpp>
#include "engine/tools/serializer.hpp"

namespace tmt {

/* Act as a tag */
struct InspectOnly {};

inline constexpr InspectOnly inspect_only {};

}  // namespace tmt

template <typename T>
requires visit_struct::traits::is_visitable<T, ImReflect::Detail::ImContext>::value
inline JsonReflect::json tag_invoke(JsonReflect::serialize_t, const T& value, tmt::InspectOnly) {
    /* serialize only the fields that are present in the inspector */
    return JsonReflect::Detail::to_json_visitable<ImReflect::Detail::ImContext>(value);
}