#pragma once

#include "engine/engine.hpp"
#include "engine/core/resource.hpp"
#include "engine/core/resources.hpp"
#include "engine/tools/serializer.hpp"

template <typename T>
tmt::json tag_invoke(JsonReflect::serialize_t, const tmt::ResourceRef<T>& value) {
    JsonReflect::json j;
    j["file_location"] = JsonReflect::to_json(value.file_location);
    if constexpr (JsonReflect::Detail::has_to_json_v<T>) {
        j["data"] = JsonReflect::to_json(value.resource);
    }
    return j;
}

template <typename T>
void tag_invoke(JsonReflect::deserialize_t, const JsonReflect::json& j, tmt::ResourceRef<T>& value) {
    JsonReflect::from_json(j.at("file_location"), value.file_location);
    if constexpr (std::derived_from<T, tmt::FileResource>) {
        value.resource = tmt::engine.resources.load_resource<T>(value.file_location).resource;
    } else if constexpr (requires { typename T::is_runtime_resource; }) {
        value.resource = tmt::engine.resources.copy_resource<T>(value.file_location).resource;
    } else {
        static_assert(svh::always_false<T>::value, "JsonSerializer Error: Cannot deserialize ResourceRef<T> where T is not a FileResource or RuntimeResource");
    }

    if constexpr (JsonReflect::Detail::has_from_json_v<T>) {
        JsonReflect::from_json(j.at("data"), value.resource);
    }
}