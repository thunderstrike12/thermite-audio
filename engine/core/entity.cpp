#include "entity.hpp"
#include "engine/core/components/transform.hpp"
#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"

namespace tmt {

template <typename R>
    requires TypeRange<R, Entity>
std::set<Entity> EntityHelper::upper_parents(const R& container) {
    std::set<Entity> root_parents;

    for (const auto& entity : container) {
        if (!EntityHelper::is_valid(entity)) continue;

        Entity current = entity;

        // Traverse up the parent chain to find the root
        while (EntityHelper::is_valid(current)) {
            const auto& transform = engine.ecs.get_component<Transform>(current);

            if (!transform.has_parent()) {
                // Found a root parent (no parent)
                root_parents.insert(current);
                break;
            }

            Entity parent = transform.get_parent();
            if (!EntityHelper::is_valid(parent)) {
                // Parent is invalid, treat current as root
                root_parents.insert(current);
                break;
            }

            current = parent;
        }
    }

    return root_parents;
}

template std::set<Entity> EntityHelper::upper_parents(const std::vector<Entity>&);
template std::set<Entity> EntityHelper::upper_parents(const std::set<Entity>&);
template std::set<Entity> EntityHelper::upper_parents(const std::unordered_set<Entity>&);

}  // namespace tmt