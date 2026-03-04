#include "gameplay.hpp"
#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/systems/gameplay/game_component.hpp"
#include "engine/core/components/component_collection.hpp"
#include "engine/systems/gameplay/game_component_registry.hpp"
#include "engine/tools/profiler.hpp"

namespace tmt {

template <typename Func>
void for_each_component(Func&& func) {
    /* using regsitry to also get disabled entities */
    const auto group = engine.ecs.get_registry().group<ComponentCollection>();
    if (group.empty()) return;

    const auto& components = engine.component_registry.get_registered_components();
    if (components.empty()) return;

    for (const auto& [component_index, info] : components) {
        TMT_ZONE_SCOPED_STRING(info.name);
        for (auto&& [entity, group_collection] : group.each()) {
            const bool has_component = group_collection.has_component(component_index);
            if (has_component == false) continue;

            IGameComponent& component = group_collection.get_component(component_index);
            func(entity, component);
        }
    }
}

void Gameplay::on_start() {
    TMT_ZONE_SCOPED_NS("Start Gameplay Components");
    for_each_component([](const Entity entity, IGameComponent& component) {
        component.start();
        component.started = true;
    });
}

void Gameplay::on_update(const tmt::FrameData& time) {
    TMT_ZONE_SCOPED_NS("Update Gameplay Components");
    for_each_component([&](const Entity entity, IGameComponent& component) {

        if (to_enable.contains(entity)) {
            component.on_entity_enabled();
            to_enable.erase(entity);
        }

        if (to_disable.contains(entity)) {
            component.on_entity_disabled();
            to_disable.erase(entity);
        }

        /* update */
        if (component.started == false) {
            component.start();
            component.started = true;
        }

        const bool enabled = engine.ecs.is_enabled(entity);
        if (enabled && component.started) {
            component.update(time);
        }
    });
}

void Gameplay::on_end() {
    TMT_ZONE_SCOPED_NS("End Gameplay Components");
    for_each_component([](const Entity entity, IGameComponent& component) {
        /* end */
        if (component.started) {
            component.end();
            component.started = false;
        }
    });

    clear_component_instances();
}

void Gameplay::on_fixed_update(const tmt::FrameData& time) {
    TMT_ZONE_SCOPED_NS("Fixed Update Gameplay Components");
    for_each_component([&time](const Entity entity, IGameComponent& component) {
        /* fixed update */
        const bool enabled = engine.ecs.is_enabled(entity);
        if (enabled && component.started) {
            component.fixed_update(time);
        }
    });
}

void Gameplay::register_component_instance(const ComponentIndex& index, std::weak_ptr<IGameComponent> component) {
    component_instances[index].push_back(component);
}

const std::vector<std::weak_ptr<IGameComponent>>& Gameplay::get_component_instances(const ComponentIndex& index) const {
    static const std::vector<std::weak_ptr<IGameComponent>> empty_vector {};
    if (component_instances.contains(index) == false) {
        return empty_vector;
    }
    return component_instances.at(index);
}

void Gameplay::on_pre_unload_scene() {
    clear_component_instances();
}

void Gameplay::on_enable_entity(const Entity& entity) {
    if (engine.game_controller.is_playing() == false) return;
    to_enable.insert(entity);
}

void Gameplay::on_disable_entity(const Entity& entity) {
    if (engine.game_controller.is_playing() == false) return;
    to_disable.insert(entity);
}

}  // namespace tmt