#include <iostream>

#include "engine/engine.hpp"

#include "engine/core/logger.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/components/transform.hpp"
#include "engine/core/components/name.hpp"
#include "engine/tools/fmt_defines.hpp"

void ecs_testing();

int main(int argc, char** argv) {
#ifdef THERMITE_EDITOR
    std::cout << "Thermite Editor included.\n";
#else
    std::cout << "Thermite Editor not included.\n";
#endif

    tmt::ApplicationSpecs specs {.name = "Game", .command_args = {argc, argv}};

    tmt::engine.init(specs);

    // ECS testing
    ecs_testing();

    tmt::engine.run();

    tmt::engine.end();

    return 0;
}

void ecs_testing() {
    /* ECS testing */
    {
        tmt::Entity entity = tmt::engine.ecs.create_entity();
        tmt::Log::info("Created entity with ID: {}", entity);

        auto& name_comp = tmt::engine.ecs.add_or_get_component<tmt::Name>(entity);
        name_comp.name = "Player";

        tmt::Log::info("Added Name component: {}", name_comp);

        auto& transform = tmt::engine.ecs.add_component<tmt::Transform>(entity);

        tmt::Log::info("Added Transform component: {}", transform);

        auto same_name = tmt::engine.ecs.get_component<tmt::Name>(entity);
        tmt::Log::info("Retrieved Name component: {}", same_name);

        tmt::engine.ecs.remove_component<tmt::Name>(entity);
        tmt::Log::info("Removed Name component from entity ID: {}", entity);

        auto* try_name = tmt::engine.ecs.try_get_component<tmt::Name>(entity);
        if (try_name == nullptr) {
            tmt::Log::info("Name component correctly not found after removal.");
        } else {
            tmt::Log::warn("Name component was found when it should have been removed!");
        }

        tmt::engine.ecs.destroy_entity(entity);
    }
    {
        tmt::Entity entity = tmt::engine.ecs.create_entity();
        tmt::Log::info("Created entity with ID: {}", entity);

        auto comps = tmt::engine.ecs.add_component<tmt::Name, tmt::Transform>(entity);
        std::get<tmt::Name>(comps).name = "Enemy";
        tmt::Log::info("Added multiple components: {}, {}", std::get<tmt::Name>(comps), std::get<tmt::Transform>(comps));

        auto [name_comp, transform_comp] = tmt::engine.ecs.get_component<tmt::Name, tmt::Transform>(entity);
        tmt::Log::info("Retrieved multiple components: {}, {}", name_comp, transform_comp);

        tmt::engine.ecs.remove_component<tmt::Name, tmt::Transform>(entity);
        tmt::Log::info("Removed multiple components from entity ID: {}", entity);

        auto [try_name, try_transform] = tmt::engine.ecs.try_get_component<tmt::Name, tmt::Transform>(entity);
        if (try_name == nullptr && try_transform == nullptr) {
            tmt::Log::info(
                "Both Name and Transform components correctly not found after "
                "removal."
            );
        } else {
            tmt::Log::warn(
                "One or both components were found when they should have been "
                "removed!"
            );
        }

        tmt::engine.ecs.destroy_entity(entity);
    }
}