#pragma once
#include <entt/entt.hpp>
#include <string>

#include "engine/tools/fmt/helper.hpp"

namespace tmt {
using Entity = entt::entity;

struct EntityHelper {
    static inline std::string to_string(Entity entity) { return std::to_string(static_cast<uint32_t>(entity)); }
    static inline Entity from_string(const std::string& string) { return static_cast<Entity>(std::stoul(string)); }
    static inline bool is_valid(Entity entity) { return entity != entt::null; }
};
}  // namespace tmt

FMT_LOGGING(tmt::Entity, "{}", tmt::EntityHelper::to_string(obj));