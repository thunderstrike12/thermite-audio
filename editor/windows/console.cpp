#include "console.hpp"

#include "engine/engine.hpp"
#include "engine/core/logger.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/io.hpp"
#include "engine/core/polyline.hpp"
#include "engine/core/components/camera.hpp"
#include "engine/core/input/input.hpp"

using namespace tmt;
namespace {
glm::vec3 draw_cursor {0.0f};

}
// temporary bandaid fix to the default parameter problem
namespace csys {
inline ItemLog& operator<<(ItemLog& log, const glm::vec3& v) {
    log << "(" << v.x << ", " << v.y << ", " << v.z << ")";
    return log;
}
}  // namespace csys
void Console::display() { console.DrawContent(); }

void Console::on_editor_start() {
    sink = std::make_shared<ConsoleSink_st>(console.System());
    sink->set_pattern("[%n] %v");
    Log::add_sink(sink);

    register_variables();
    register_commands();
    register_script("example_script", (std::filesystem::path(tmt::IO::get_sub_location_path(tmt::IO::Location::EDITOR)) / "console_scripts" / "example.script").string());
}

void Console::on_editor_update(const FrameData& time) {}

void Console::on_editor_end() { Log::remove_sink(sink); }
void Console::register_variables() { register_variable("draw_cursor", draw_cursor, csys::Arg<float>("x"), csys::Arg<float>("y"), csys::Arg<float>("z")); }

void Console::register_commands() {
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
            auto& registry = engine.ecs.get_registry();
            for (auto entity : registry.view<Name>()) {
                if (registry.get<Name>(entity).name == name) {
                    engine.ecs.destroy_entity(entity);
                    Log::info("Destroyed '{}'", name);
                    return;
                }
            }
            Log::error("Entity '{}' not found", name);
        },
        csys::Arg<csys::String>("name")
    );

    register_command("list", "List all entities", []() {
        auto& registry = engine.ecs.get_registry();
        for (auto entity : registry.view<Name>()) {
            Log::info("[{}] {}", static_cast<uint32_t>(entity), registry.get<Name>(entity).name);
        }
    });

    // Input related commands
    register_command(
        "warp_mouse", "Moves the cursor instantly to the given position relative to the window",
        [](float x, float y) {
            auto mouse_pos = glm::vec2 {x, y};
            tmt::engine.input.warp_mouse(mouse_pos);

            Log::info("Mouse position is moved to {}", mouse_pos.x, mouse_pos.y);
        },

        csys::Arg<float>("mouse_x"), csys::Arg<float>("mouse_y")

    );
    // Drawing state
    register_command(
        "draw_color", "Sets the color used by draw commands: draw_color <r> <g> <b>", [](float r, float g, float b) { engine.polyline.use_color(r, g, b); }, csys::Arg<float>("r"),
        csys::Arg<float>("g"), csys::Arg<float>("b")
    );

    register_command("draw_width", "Sets the line width used by draw commands: draw_width <width>", [](float width) { engine.polyline.use_line_width(width); }, csys::Arg<float>("width"));

    // Shape commands
    register_command(
        "draw_line", "Draws a line from cursor to the specified point: draw_line <x> <y> <z> <time>",
        [](float x, float y, float z, float time) { engine.polyline.draw_line(draw_cursor, {x, y, z}, time); }, csys::Arg<float>("x"), csys::Arg<float>("y"), csys::Arg<float>("z"),
        csys::Arg<float>("time")
    );

    register_command(
        "draw_circle", "Draws a camera-facing circle at cursor position: draw_circle <radius> <segments> <time>",
        [](float radius, int segments, float time) { engine.polyline.draw_circle(draw_cursor, radius, segments, time); }, csys::Arg<float>("radius"), csys::Arg<int>("segments"),
        csys::Arg<float>("time")
    );

    register_command(
        "draw_sphere", "Draws a wireframe sphere at cursor position: draw_sphere <radius> <segments> <time>",
        [](float radius, int segments, float time) { engine.polyline.draw_sphere(draw_cursor, radius, segments, time); }, csys::Arg<float>("radius"), csys::Arg<int>("segments"),
        csys::Arg<float>("time")
    );

    register_command(
        "draw_aabb", "Draws an axis-aligned bounding box centered at cursor: draw_aabb <hx> <hy> <hz> <time>",
        [](float hx, float hy, float hz, float time) {
            glm::vec3 half {hx, hy, hz};
            engine.polyline.draw_aabb(draw_cursor - half, draw_cursor + half, time);
        },
        csys::Arg<float>("hx"), csys::Arg<float>("hy"), csys::Arg<float>("hz"), csys::Arg<float>("time")
    );

    register_command(
        "draw_arrow", "Draws an arrow from cursor in specified direction: draw_arrow <dirX> <dirY> <dirZ> <length> <time>",
        [](float dirX, float dirY, float dirZ, float length, float time) { engine.polyline.draw_arrow(draw_cursor, glm::normalize(glm::vec3 {dirX, dirY, dirZ}), length, time); },
        csys::Arg<float>("dirX"), csys::Arg<float>("dirY"), csys::Arg<float>("dirZ"), csys::Arg<float>("length"), csys::Arg<float>("time")
    );

    register_command(
        "draw_cone", "Draws a wireframe cone from cursor in specified direction: draw_cone <dirX> <dirY> <dirZ> <angle> <length> <segments> <time>",
        [](float dirX, float dirY, float dirZ, float angle, float length, int segments, float time) {
            engine.polyline.draw_cone(draw_cursor, glm::normalize(glm::vec3 {dirX, dirY, dirZ}), angle, length, segments, time);
        },
        csys::Arg<float>("dirX"), csys::Arg<float>("dirY"), csys::Arg<float>("dirZ"), csys::Arg<float>("angle"), csys::Arg<float>("length"), csys::Arg<int>("segments"),
        csys::Arg<float>("time")
    );

    register_command(
        "draw_tube", "Draws a wireframe tube from cursor to specified point: draw_tube <x> <y> <z> <radius> <segments> <time>",
        [](float x, float y, float z, float radius, int segments, float time) { engine.polyline.draw_tube(draw_cursor, {x, y, z}, radius, segments, time); }, csys::Arg<float>("x"),
        csys::Arg<float>("y"), csys::Arg<float>("z"), csys::Arg<float>("radius"), csys::Arg<int>("segments"), csys::Arg<float>("time")
    );

    register_command(
        "draw_obb", "Draws an oriented bounding box centered at cursor: draw_obb <hx> <hy> <hz> <axisX> <axisY> <axisZ> <angle> <time>",
        [](float hx, float hy, float hz, float axisX, float axisY, float axisZ, float angle, float time) {
            glm::vec3 axis = glm::normalize(glm::vec3 {axisX, axisY, axisZ});
            engine.polyline.draw_obb(draw_cursor, {hx, hy, hz}, glm::angleAxis(angle, axis), time);
        },
        csys::Arg<float>("hx"), csys::Arg<float>("hy"), csys::Arg<float>("hz"), csys::Arg<float>("axisX"), csys::Arg<float>("axisY"), csys::Arg<float>("axisZ"), csys::Arg<float>("angle"),
        csys::Arg<float>("time")
    );

    register_command(
        "draw_bone", "Draws a bone shape at cursor for skeleton visualization: draw_bone <length> <axisX> <axisY> <axisZ> <angle> <time>",
        [](float length, float axisX, float axisY, float axisZ, float angle, float time) {
            glm::vec3 axis = glm::normalize(glm::vec3 {axisX, axisY, axisZ});
            engine.polyline.draw_bone(draw_cursor, glm::angleAxis(angle, axis), length, time);
        },
        csys::Arg<float>("length"), csys::Arg<float>("axisX"), csys::Arg<float>("axisY"), csys::Arg<float>("axisZ"), csys::Arg<float>("angle"), csys::Arg<float>("time")
    );

    register_command(
        "draw_text", "Draws billboard text at cursor position: draw_text <text> <size> <time>",
        [](const std::string& text, float size, float time) { engine.polyline.draw_text(draw_cursor, text, size, time); }, csys::Arg<csys::String>("text"), csys::Arg<float>("size"),
        csys::Arg<float>("time")
    );
}
