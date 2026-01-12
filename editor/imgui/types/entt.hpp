#pragma once
#include <entt/entt.hpp>
#include <ImReflect.hpp>

inline void tag_invoke(ImReflect::ImInput_t, const char* label, entt::entity& value, ImSettings& settings, ImResponse& response) {
    /* Cast to uint32_t */
    auto temp = static_cast<uint32_t>(value);
    ImReflect::Input(label, temp, settings, response);
    value = static_cast<entt::entity>(temp);
}

inline void tag_invoke(ImReflect::ImInput_t, const char* label, const entt::entity& value, ImSettings& settings, ImResponse& response) {
    /* Cast to uint32_t */
    const auto temp = static_cast<uint32_t>(value);
    ImReflect::Input(label, temp, settings, response);
    /* value is const, can not be modified */
}