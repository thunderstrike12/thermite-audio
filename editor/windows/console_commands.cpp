#include "console.hpp"

#include "engine/engine.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/entity.hpp"
#include "engine/core/logger.hpp"
#include "engine/core/polyline.hpp"
#include "engine/core/scenes.hpp"
#include "engine/core/components/camera.hpp"
#include "engine/core/components/name.hpp"
#include "engine/core/components/transform.hpp"
#include "engine/core/input/input.hpp"
#include "engine/core/renderer/renderer.hpp"
#include "extern/imgui-console/include/csys/arguments.h"

using namespace tmt;

namespace {

// TODO this might be useful in ECS?
template <typename... Components>
Entity find_entity(const std::string& name) {
    auto& registry = engine.ecs.get_registry();
    for (auto entity : registry.view<Name, Components...>()) {
        if (registry.get<Name>(entity).name == name) {
            return entity;
        }
    }
    return entt::null;
}

// gets the description via magic enum
std::string build_display_mode_help() {
    std::string help = "Set display mode: display_mode [mode=0]";
    for (auto [value, name] : magic_enum::enum_entries<DisplayMode>()) {
        help += fmt::format("\n  {} = {}", static_cast<int>(value), name);
    }
    return help;
}

}  // namespace

void Console::register_commands() {
    // =========================================================================
    // Entity Management
    // =========================================================================

    register_command(
        "create", "Create entity: create <name>",
        [](const std::string& name) {
            Entity entity = engine.ecs.create_entity(name);
            Log::info("Created '{}' [{}]", name, static_cast<uint32_t>(entity));
        },
        csys::Arg<csys::String>("name")
    );

    register_command(
        "destroy", "Destroy entity: destroy <name>",
        [](const std::string& name) {
            if (auto entity = find_entity(name); entity != entt::null) {
                engine.ecs.destroy_entity(entity);
                Log::info("Destroyed '{}'", name);
            } else {
                Log::error("Entity '{}' not found", name);
            }
        },
        csys::Arg<csys::String>("name")
    );

    register_command("list", "List all entities: list", []() {
        auto& registry = engine.ecs.get_registry();
        for (auto entity : registry.view<Name>()) {
            Log::info("[{}] {}", static_cast<uint32_t>(entity), registry.get<Name>(entity).name);
        }
    });

    // =========================================================================
    // Transform / Teleportation
    // =========================================================================

    // Active camera commands (short names for convenience)
    register_command(
        "tp", "Teleport active camera: tp <x> <y> <z>",
        [](float x, float y, float z) {
            auto entity = Camera::get_active_camera();
            if (entity == entt::null) {
                Log::error("No active camera");
                return;
            }
            engine.ecs.get_component<Transform>(entity).set_world_position({ x, y, z });
            Log::info("Teleported to ({}, {}, {})", x, y, z);
        },
        csys::Arg<float>("x"), csys::Arg<float>("y"), csys::Arg<float>("z")
    );

    register_command(
        "tp_to", "Teleport active camera to entity: tp_to <target>",
        [](const std::string& target) {
            auto entity = Camera::get_active_camera();
            if (entity == entt::null) {
                Log::error("No active camera");
                return;
            }
            auto target_entity = find_entity<Transform>(target);
            if (target_entity == entt::null) {
                Log::error("Target '{}' not found", target);
                return;
            }
            auto target_pos = engine.ecs.get_component<Transform>(target_entity).get_world_position();
            engine.ecs.get_component<Transform>(entity).set_world_position(target_pos);
            Log::info("Teleported to '{}'", target);
        },
        csys::Arg<csys::String>("target")
    );

    register_command(
        "tp_facing", "Teleport active camera facing point: tp_facing <x> <y> <z> <look_x> <look_y> <look_z>",
        [](float x, float y, float z, float lx, float ly, float lz) {
            auto entity = Camera::get_active_camera();
            if (entity == entt::null) {
                Log::error("No active camera");
                return;
            }
            auto& transform = engine.ecs.get_component<Transform>(entity);
            transform.set_world_position({ x, y, z });
            transform.look_at({ lx, ly, lz }, { 0.0f, 1.0f, 0.0f });
            Log::info("Teleported to ({}, {}, {}) facing ({}, {}, {})", x, y, z, lx, ly, lz);
        },
        csys::Arg<float>("x"), csys::Arg<float>("y"), csys::Arg<float>("z"), csys::Arg<float>("look_x"), csys::Arg<float>("look_y"), csys::Arg<float>("look_z")
    );

    register_command(
        "tp_facing_to", "Teleport active camera to entity facing entity: tp_facing_to <target> <look_at>",
        [](const std::string& target, const std::string& look_at_name) {
            auto entity = Camera::get_active_camera();
            if (entity == entt::null) {
                Log::error("No active camera");
                return;
            }
            auto target_entity = find_entity<Transform>(target);
            if (target_entity == entt::null) {
                Log::error("Target '{}' not found", target);
                return;
            }
            auto look_at_entity = find_entity<Transform>(look_at_name);
            if (look_at_entity == entt::null) {
                Log::error("Look-at target '{}' not found", look_at_name);
                return;
            }
            auto& transform = engine.ecs.get_component<Transform>(entity);
            transform.set_world_position(engine.ecs.get_component<Transform>(target_entity).get_world_position());
            transform.look_at(engine.ecs.get_component<Transform>(look_at_entity).get_world_position(), { 0.0f, 1.0f, 0.0f });
            Log::info("Teleported to '{}' facing '{}'", target, look_at_name);
        },
        csys::Arg<csys::String>("target"), csys::Arg<csys::String>("look_at")
    );

    // Named entity commands
    register_command(
        "teleport", "Teleport entity to coordinates: teleport <entity> <x> <y> <z>",
        [](const std::string& name, float x, float y, float z) {
            auto entity = find_entity<Transform>(name);
            if (entity == entt::null) {
                Log::error("Entity '{}' not found", name);
                return;
            }
            engine.ecs.get_component<Transform>(entity).set_world_position({ x, y, z });
            Log::info("Teleported '{}' to ({}, {}, {})", name, x, y, z);
        },
        csys::Arg<csys::String>("entity"), csys::Arg<float>("x"), csys::Arg<float>("y"), csys::Arg<float>("z")
    );

    register_command(
        "teleport_to", "Teleport entity to entity: teleport_to <entity> <target>",
        [](const std::string& name, const std::string& target) {
            auto entity = find_entity<Transform>(name);
            if (entity == entt::null) {
                Log::error("Entity '{}' not found", name);
                return;
            }
            auto target_entity = find_entity<Transform>(target);
            if (target_entity == entt::null) {
                Log::error("Target '{}' not found", target);
                return;
            }
            auto target_pos = engine.ecs.get_component<Transform>(target_entity).get_world_position();
            engine.ecs.get_component<Transform>(entity).set_world_position(target_pos);
            Log::info("Teleported '{}' to '{}'", name, target);
        },
        csys::Arg<csys::String>("entity"), csys::Arg<csys::String>("target")
    );

    register_command(
        "teleport_facing", "Teleport entity facing point: teleport_facing <entity> <x> <y> <z> <look_x> <look_y> <look_z>",
        [](const std::string& name, float x, float y, float z, float lx, float ly, float lz) {
            auto entity = find_entity<Transform>(name);
            if (entity == entt::null) {
                Log::error("Entity '{}' not found", name);
                return;
            }
            auto& transform = engine.ecs.get_component<Transform>(entity);
            transform.set_world_position({ x, y, z });
            transform.look_at({ lx, ly, lz }, { 0.0f, 1.0f, 0.0f });
            Log::info("Teleported '{}' to ({}, {}, {}) facing ({}, {}, {})", name, x, y, z, lx, ly, lz);
        },
        csys::Arg<csys::String>("entity"), csys::Arg<float>("x"), csys::Arg<float>("y"), csys::Arg<float>("z"), csys::Arg<float>("look_x"), csys::Arg<float>("look_y"),
        csys::Arg<float>("look_z")
    );

    register_command(
        "teleport_facing_to", "Teleport entity to entity facing entity: teleport_facing_to <entity> <target> <look_at>",
        [](const std::string& name, const std::string& target, const std::string& look_at_name) {
            auto entity = find_entity<Transform>(name);
            if (entity == entt::null) {
                Log::error("Entity '{}' not found", name);
                return;
            }
            auto target_entity = find_entity<Transform>(target);
            if (target_entity == entt::null) {
                Log::error("Target '{}' not found", target);
                return;
            }
            auto look_at_entity = find_entity<Transform>(look_at_name);
            if (look_at_entity == entt::null) {
                Log::error("Look-at target '{}' not found", look_at_name);
                return;
            }
            auto& transform = engine.ecs.get_component<Transform>(entity);
            transform.set_world_position(engine.ecs.get_component<Transform>(target_entity).get_world_position());
            transform.look_at(engine.ecs.get_component<Transform>(look_at_entity).get_world_position(), { 0.0f, 1.0f, 0.0f });
            Log::info("Teleported '{}' to '{}' facing '{}'", name, target, look_at_name);
        },
        csys::Arg<csys::String>("entity"), csys::Arg<csys::String>("target"), csys::Arg<csys::String>("look_at")
    );

    // =========================================================================
    // Game Flow
    // =========================================================================

    register_command("play", "Start the game: play", []() {
        engine.game_controller.start_game();
        Log::info("Game started");
    });

    register_command("stop", "Stop the game: stop", []() {
        engine.game_controller.end_game();
        Log::info("Game stopped");
    });

    register_command("pause", "Pause the game: pause", []() {
        engine.game_controller.pause_game();
        Log::info("Game paused");
    });

    register_command("resume", "Resume the game: resume", []() {
        engine.game_controller.resume_game();
        Log::info("Game resumed");
    });

    register_command("toggle_pause", "Toggle pause state: toggle_pause", []() {
        if (engine.game_controller.is_paused()) {
            engine.game_controller.resume_game();
            Log::info("Game resumed");
        } else {
            engine.game_controller.pause_game();
            Log::info("Game paused");
        }
    });

    // =========================================================================
    // Scene Management
    // =========================================================================

    register_command("scene_list", "List all registered scenes: scene_list", []() {
        const auto& active = engine.scenes.get_active_scene();
        for (const auto& [type_index, scene_info] : engine.scenes.get_registered_scenes()) {
            bool is_active = active && typeid(*active) == type_index;
            Log::info("{} {}", is_active ? "*" : " ", scene_info.name);
        }
    });

    register_command(
        "scene_load", "Load a scene: scene_load <name>",
        [](const std::string& name) {
            for (const auto& [type_index, scene_info] : engine.scenes.get_registered_scenes()) {
                if (scene_info.name == name) {
                    engine.scenes.enqueue_scene(type_index);
                    Log::info("Loading scene: {}", name);
                    return;
                }
            }
            Log::error("Scene '{}' not found", name);
        },
        csys::Arg<csys::String>("name")
    );

    register_command("scene_save", "Save the current scene: scene_save", []() {
        engine.scenes.serialize_active_scene();
        Log::info("Scene saved");
    });

    register_command("scene_reload", "Reload the current scene: scene_reload", []() {
        if (engine.scenes.get_active_scene()) {
            engine.scenes.enqueue_scene(engine.scenes.get_active_scene_type());
            Log::info("Reloading scene");
        } else {
            Log::error("No active scene");
        }
    });

    // =========================================================================
    // Time Control
    // =========================================================================

    // TODO
    /* register_command(
         "timescale", "Set time scale: timescale [scale=1]",
         [](float scale) {
             engine.set_scale(scale);
             Log::info("Time scale: {}", scale);
         },
         csys::Arg<float>("scale", 1.0f)
     );*/

    // =========================================================================
    // Renderer
    // =========================================================================

    register_command(
        "display_mode", build_display_mode_help(),
        [](int mode) {
            auto display_mode = magic_enum::enum_cast<DisplayMode>(mode);
            if (!display_mode.has_value()) {
                Log::error("Invalid display mode {}. Valid range: 0-{}", mode, magic_enum::enum_count<DisplayMode>() - 1);
                return;
            }
            engine.renderer.display_mode = *display_mode;
            Log::info("Display mode: {} ({})", magic_enum::enum_name(*display_mode), mode);
        },
        csys::Arg<int>("mode", 0)
    );

    // =========================================================================
    // Input
    // =========================================================================

    register_command(
        "warp_mouse", "Warp mouse cursor: warp_mouse <x> <y>",
        [](float x, float y) {
            engine.input.warp_mouse({ x, y });
            Log::info("Mouse warped to ({}, {})", x, y);
        },
        csys::Arg<float>("x"), csys::Arg<float>("y")
    );

    // =========================================================================
    // Utility
    // =========================================================================

    // TODO add profiling info here and stats
    register_command("fps", "Show current FPS: fps", []() { Log::info("FPS: {:.1f} ({:.2f}ms)", 1.0f / engine.frame_data().delta_time, engine.frame_data().delta_time * 1000.0f); });

    // =========================================================================
    // Debug Drawing - State
    // =========================================================================

    register_command(
        "draw_color", "Set draw color: draw_color <r> <g> <b>", [](float r, float g, float b) { engine.polyline.use_color(r, g, b); }, csys::Arg<float>("r"), csys::Arg<float>("g"),
        csys::Arg<float>("b")
    );

    register_command("draw_width", "Set line width: draw_width [width=1]", [](float width) { engine.polyline.use_line_width(width); }, csys::Arg<float>("width", 1.0f));

    // =========================================================================
    // Debug Drawing - Shapes
    // =========================================================================

    register_command(
        "draw_line", "Draw line: draw_line <x1> <y1> <z1> <x2> <y2> <z2> [time=5]",
        [](float x1, float y1, float z1, float x2, float y2, float z2, float time) { engine.polyline.draw_line({ x1, y1, z1 }, { x2, y2, z2 }, time); }, csys::Arg<float>("x1"),
        csys::Arg<float>("y1"), csys::Arg<float>("z1"), csys::Arg<float>("x2"), csys::Arg<float>("y2"), csys::Arg<float>("z2"), csys::Arg<float>("time", 5.0f)
    );

    register_command(
        "draw_circle", "Draw circle: draw_circle <x> <y> <z> <radius> [segments=32] [time=5]",
        [](float x, float y, float z, float radius, int segments, float time) { engine.polyline.draw_circle({ x, y, z }, radius, segments, time); }, csys::Arg<float>("x"),
        csys::Arg<float>("y"), csys::Arg<float>("z"), csys::Arg<float>("radius"), csys::Arg<int>("segments", 32), csys::Arg<float>("time", 5.0f)
    );

    register_command(
        "draw_sphere", "Draw sphere: draw_sphere <x> <y> <z> <radius> [segments=16] [time=5]",
        [](float x, float y, float z, float radius, int segments, float time) { engine.polyline.draw_sphere({ x, y, z }, radius, segments, time); }, csys::Arg<float>("x"),
        csys::Arg<float>("y"), csys::Arg<float>("z"), csys::Arg<float>("radius"), csys::Arg<int>("segments", 16), csys::Arg<float>("time", 5.0f)
    );

    register_command(
        "draw_aabb", "Draw AABB: draw_aabb <minX> <minY> <minZ> <maxX> <maxY> <maxZ> [time=5]",
        [](float minX, float minY, float minZ, float maxX, float maxY, float maxZ, float time) { engine.polyline.draw_aabb({ minX, minY, minZ }, { maxX, maxY, maxZ }, time); },
        csys::Arg<float>("minX"), csys::Arg<float>("minY"), csys::Arg<float>("minZ"), csys::Arg<float>("maxX"), csys::Arg<float>("maxY"), csys::Arg<float>("maxZ"),
        csys::Arg<float>("time", 5.0f)
    );

    register_command(
        "draw_arrow", "Draw arrow: draw_arrow <x> <y> <z> <dirX> <dirY> <dirZ> [length=1] [time=5]",
        [](float x, float y, float z, float dirX, float dirY, float dirZ, float length, float time) {
            engine.polyline.draw_arrow({ x, y, z }, glm::normalize(glm::vec3 { dirX, dirY, dirZ }), length, time);
        },
        csys::Arg<float>("x"), csys::Arg<float>("y"), csys::Arg<float>("z"), csys::Arg<float>("dirX"), csys::Arg<float>("dirY"), csys::Arg<float>("dirZ"), csys::Arg<float>("length", 1.0f),
        csys::Arg<float>("time", 5.0f)
    );

    register_command(
        "draw_cone", "Draw cone: draw_cone <x> <y> <z> <dirX> <dirY> <dirZ> [angle=45] [length=1] [segments=16] [time=5]",
        [](float x, float y, float z, float dirX, float dirY, float dirZ, float angle, float length, int segments, float time) {
            engine.polyline.draw_cone({ x, y, z }, glm::normalize(glm::vec3 { dirX, dirY, dirZ }), angle, length, segments, time);
        },
        csys::Arg<float>("x"), csys::Arg<float>("y"), csys::Arg<float>("z"), csys::Arg<float>("dirX"), csys::Arg<float>("dirY"), csys::Arg<float>("dirZ"), csys::Arg<float>("angle", 45.0f),
        csys::Arg<float>("length", 1.0f), csys::Arg<int>("segments", 16), csys::Arg<float>("time", 5.0f)
    );

    register_command(
        "draw_tube", "Draw tube: draw_tube <x1> <y1> <z1> <x2> <y2> <z2> [radius=0.1] [segments=16] [time=5]",
        [](float x1, float y1, float z1, float x2, float y2, float z2, float radius, int segments, float time) {
            engine.polyline.draw_tube({ x1, y1, z1 }, { x2, y2, z2 }, radius, segments, time);
        },
        csys::Arg<float>("x1"), csys::Arg<float>("y1"), csys::Arg<float>("z1"), csys::Arg<float>("x2"), csys::Arg<float>("y2"), csys::Arg<float>("z2"), csys::Arg<float>("radius", 0.1f),
        csys::Arg<int>("segments", 16), csys::Arg<float>("time", 5.0f)
    );

    register_command(
        "draw_obb", "Draw OBB: draw_obb <x> <y> <z> <hx> <hy> <hz> [axisX=0] [axisY=1] [axisZ=0] [angle=0] [time=5]",
        [](float x, float y, float z, float hx, float hy, float hz, float axisX, float axisY, float axisZ, float angle, float time) {
            glm::vec3 axis = glm::normalize(glm::vec3 { axisX, axisY, axisZ });
            engine.polyline.draw_obb({ x, y, z }, { hx, hy, hz }, glm::angleAxis(angle, axis), time);
        },
        csys::Arg<float>("x"), csys::Arg<float>("y"), csys::Arg<float>("z"), csys::Arg<float>("hx"), csys::Arg<float>("hy"), csys::Arg<float>("hz"), csys::Arg<float>("axisX", 0.0f),
        csys::Arg<float>("axisY", 1.0f), csys::Arg<float>("axisZ", 0.0f), csys::Arg<float>("angle", 0.0f), csys::Arg<float>("time", 5.0f)
    );

    register_command(
        "draw_bone", "Draw bone: draw_bone <x> <y> <z> [length=1] [axisX=0] [axisY=1] [axisZ=0] [angle=0] [time=5]",
        [](float x, float y, float z, float length, float axisX, float axisY, float axisZ, float angle, float time) {
            glm::vec3 axis = glm::normalize(glm::vec3 { axisX, axisY, axisZ });
            engine.polyline.draw_bone({ x, y, z }, glm::angleAxis(angle, axis), length, time);
        },
        csys::Arg<float>("x"), csys::Arg<float>("y"), csys::Arg<float>("z"), csys::Arg<float>("length", 1.0f), csys::Arg<float>("axisX", 0.0f), csys::Arg<float>("axisY", 1.0f),
        csys::Arg<float>("axisZ", 0.0f), csys::Arg<float>("angle", 0.0f), csys::Arg<float>("time", 5.0f)
    );

    register_command(
        "draw_text", "Draw text: draw_text <x> <y> <z> <text> [size=1] [time=5]",
        [](float x, float y, float z, const std::string& text, float size, float time) { engine.polyline.draw_text({ x, y, z }, text, size, time); }, csys::Arg<float>("x"),
        csys::Arg<float>("y"), csys::Arg<float>("z"), csys::Arg<csys::String>("text"), csys::Arg<float>("size", 1.0f), csys::Arg<float>("time", 5.0f)
    );
}
