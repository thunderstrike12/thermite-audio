#pragma once
#include "engine/tools/serializer.hpp"
#include "engine/tools/uuid.hpp"

inline JsonReflect::json tag_invoke(JsonReflect::serialize_t, const tmt::UUID& uuid) {
    //
    return uuid.str();
}

inline void tag_invoke(JsonReflect::deserialize_t, const JsonReflect::json& j, tmt::UUID& uuid) {
    //
    uuid = tmt::UUID::fromStrFactory(j.get<std::string>());
}