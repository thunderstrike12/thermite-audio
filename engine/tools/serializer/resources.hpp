#pragma once

#include <tuple>

#include "engine/engine.hpp"
#include "engine/core/resource.hpp"
#include "engine/core/resources.hpp"
#include "engine/tools/serializer.hpp"
#include "engine/core/logger.hpp"

template <typename T>
static inline tmt::json tag_invoke(JsonReflect::serialize_t, const tmt::ResourceRef<T>& value) {
    JsonReflect::json j;
    j["file_location"] = JsonReflect::to_json(value.file_location);
    if constexpr (JsonReflect::Detail::has_to_json_v<T>) {
        j["data"] = JsonReflect::to_json(value.resource);
    }
    if constexpr (requires { typename T::has_constructor_args; }) {
        if constexpr (T::has_constructor_args::value) {
            if (value) {
                j["constructor_args"] = JsonReflect::to_json(value->constructor_args);
            }
        }
    }
    return j;
}

template <typename T>
static inline void tag_invoke(JsonReflect::deserialize_t, const JsonReflect::json& j, tmt::ResourceRef<T>& value) {
    if (!j.contains("file_location")) {
        value = tmt::ResourceRef<T>();
        tmt::Log::warn(tmt::Log::Scope::ENGINE, "[Serializer] ResourceRef: Json does not contain file_location entry");
        return;
    }

    JsonReflect::from_json(j.at("file_location"), value.file_location);

    if constexpr (std::derived_from<T, tmt::FileResource>) {
        value.resource = tmt::engine.resources.load_resource<T>(value.file_location).resource;
    } else if constexpr (requires { typename T::is_runtime_resource; }) {
        /* Load the underlying file resource first */
        auto file_ref = tmt::engine.resources.load_resource<typename T::ResourceType>(value.file_location);

        if constexpr (requires { typename T::has_constructor_args; }) {
            if constexpr (T::has_constructor_args::value) {
                /* Deserialize constructor args and use them to create the runtime resource */
                typename T::ConstructorArgTypes args {};
                if (j.contains("constructor_args")) {
                    JsonReflect::from_json(j.at("constructor_args"), args);
                    auto result = std::apply(
                        [&](auto&&... unpacked_args) {
                            //
                            return tmt::engine.resources.copy_resource<T>(file_ref, std::forward<decltype(unpacked_args)>(unpacked_args)...);
                        },
                        std::move(args)
                    );
                    value.resource = result.resource;
                } else {
                    value.resource = tmt::engine.resources.copy_resource<T>(file_ref).resource;
                }
            } else {
                value.resource = tmt::engine.resources.copy_resource<T>(file_ref).resource;
            }
        } else {
            value.resource = tmt::engine.resources.copy_resource<T>(file_ref).resource;
        }
    } else {
        static_assert(svh::always_false<T>::value, "JsonSerializer Error: Cannot deserialize ResourceRef<T> where T is not a FileResource or RuntimeResource");
    }

    if (value.resource == nullptr) {
        return;
    }

    if constexpr (JsonReflect::Detail::has_from_json_v<T>) {
        if (j.contains("data")) {
            JsonReflect::from_json(j.at("data"), value.resource);
        } else {
            tmt::Log::warn(tmt::Log::Scope::ENGINE, "[Serializer] ResourceRef: Json does not contain data entry");
        }
    }
}