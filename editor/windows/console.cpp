#include "console.hpp"

#include "engine/engine.hpp"
#include "engine/core/logger.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/io.hpp"
#include "engine/core/components/camera.hpp"

using namespace tmt;

void Console::display() { console.DrawContent(); }

void Console::on_editor_start() {
    sink = std::make_shared<ConsoleSink_st>(console.System());
    sink->set_pattern("[%n] %v");
    Log::add_sink(sink);

    register_commands();
    register_script("example_script", (std::filesystem::path(tmt::IO::get_sub_location_path(tmt::IO::Location::EDITOR)) / "console_scripts" / "example.script").string());
}

void Console::on_editor_update(const FrameData& time) {}

void Console::on_editor_end() { Log::remove_sink(sink); }

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
}
