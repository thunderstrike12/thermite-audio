#pragma once
#include <entt/entt.hpp>
#include <set>
#include <string>
#include <ranges>
#include "engine/tools/fmt/helper.hpp"
#include "engine/tools/uuid.hpp"

namespace tmt {

using Entity = entt::entity;

struct DisableFlag {}; /* used to mark disabled entities, gets serialized */
struct Disable {};     /* disabled entities, does not get serialized, only used at runtime */

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

TMT_COMPONENT_NAME(tmt::DisableFlag, "Disable");
TMT_COMPONENT_SERIALIZE_EMPTY(tmt::DisableFlag);
TMT_COMPONENT_INSPECT_EMPTY(tmt::DisableFlag);
FMT_LOGGING(tmt::Entity, "{}", tmt::EntityHelper::to_string(obj));
