#pragma once
#include <entt/entt.hpp>
#include <string>
#include <ranges>
#include "engine/tools/fmt/helper.hpp"
#include "engine/tools/uuid.hpp"

namespace tmt {
using Entity = entt::entity;

template <typename R, typename T>
concept TypeRange = std::ranges::range<R> && std::same_as<std::ranges::range_value_t<R>, T>;

struct EntityHelper {
    static inline std::string to_string(Entity entity) { return std::to_string(static_cast<uint32_t>(entity)); }
    static inline Entity from_string(const std::string& string) { return static_cast<Entity>(std::stoul(string)); }
    static inline bool is_valid(Entity entity) { return entity != entt::null; }

    template <typename R>
        requires TypeRange<R, Entity>
    static inline std::set<Entity> upper_parents(const R& container);
};
}  // namespace tmt

FMT_LOGGING(tmt::Entity, "{}", tmt::EntityHelper::to_string(obj));
