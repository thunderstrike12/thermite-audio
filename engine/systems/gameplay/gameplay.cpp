#include "gameplay.hpp"
#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/components/component_collection.hpp"
#include "engine/systems/gameplay/game_component_registry.hpp"
#include "engine/tools/profiler.hpp"

namespace tmt {

template <typename Func>
void for_each_component(Func&& func) {
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
            func(component);
        }
    }
}

void Gameplay::on_start() {
    TMT_ZONE_SCOPED_NS("Start Gameplay Components");
    for_each_component([](IGameComponent& component) {
        /* start */
        component.start();
    });
}

void Gameplay::on_update(const tmt::FrameData& time) {
    TMT_ZONE_SCOPED_NS("Update Gameplay Components");
    for_each_component([&time](IGameComponent& component) {
        /* update */
        component.update(time);
    });
}

void Gameplay::on_end() {
    TMT_ZONE_SCOPED_NS("End Gameplay Components");
    for_each_component([](IGameComponent& component) {
        /* end */
        component.end();
    });
}

void Gameplay::on_fixed_update(const tmt::FrameData& time) {
    TMT_ZONE_SCOPED_NS("Fixed Update Gameplay Components");
    for_each_component([&time](IGameComponent& component) {
        /* fixed update */
        component.fixed_update(time);
    });
}

}  // namespace tmt