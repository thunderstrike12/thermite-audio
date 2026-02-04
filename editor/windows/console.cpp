#include "console.hpp"

#include "engine/core/logger.hpp"
#include "engine/core/ecs.hpp"
#include "engine/core/io.hpp"
#include "engine/core/polyline.hpp"
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
    register_scripts();
}

void Console::on_editor_update(const FrameData& time) {}

void Console::on_editor_end() { Log::remove_sink(sink); }
void Console::register_variables() {}
void Console::register_scripts() {
    const auto scripts_path = std::filesystem::path(tmt::IO::get_sub_location_path(tmt::IO::Location::EDITOR)) / "console_scripts";

    if (!std::filesystem::exists(scripts_path)) {
        Log::warn("Console scripts directory not found: {}", scripts_path.string());
        return;
    }

    for (const auto& entry : std::filesystem::directory_iterator(scripts_path)) {
        if (entry.is_regular_file() && entry.path().extension() == ".script") {
            const auto script_name = entry.path().stem().string();
            register_script(script_name, entry.path().string());
            Log::info("Registered script: {}", script_name);
        }
    }
}
