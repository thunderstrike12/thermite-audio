#pragma once
#include <named_type.hpp>
#include "JsonReflect.hpp"

template <typename T, typename Parameter, template <typename> class... Skills>
static inline JsonReflect::json tag_invoke(JsonReflect::serialize_t, const fluent::NamedType<T, Parameter, Skills...>& type) {
    // type.get();
    return JsonReflect::to_json(type.get());
}
template <typename T, typename Parameter, template <typename> class... Skills>
static inline void tag_invoke(JsonReflect::deserialize_t, const JsonReflect::json& j, fluent::NamedType<T, Parameter, Skills...>& type) {
    T value;
    JsonReflect::from_json(j, value);
    type = fluent::NamedType<T, Parameter, Skills...>(std::move(value));
}