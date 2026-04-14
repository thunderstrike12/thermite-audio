#include "prefab.hpp"
#include "prefab.hpp"

#include <imgui.h>

#include "engine/engine.hpp"
#include "engine/core/scenes.hpp"
#include "engine/core/io.hpp"
#include "engine/core/logger.hpp"
#include "engine/tools/serializer.hpp"
#include "engine/tools/serializer/ecs.hpp"
#include "engine/core/renderer/renderer.hpp"

#include "editor/core/systems/pop_up/pop_up.hpp"

namespace tmt {

void PrefabMode::display_main_menu() {
    if (!ImGui::BeginMenu("File")) return;

    if (ImGui::MenuItem("Save")) {
        save_prefab();
    }

    ImGui::EndMenu();
}

void PrefabMode::on_switch_to(const std::any& meta_data) {
    if (meta_data.has_value() == false) {
        Log::error("PrefabMode::on_switch_to called without prefab location meta data");
        return;
    }
    /* Disable TAA */
    engine.renderer.enable_taa = false;

    engine.scenes.load_scene<PrefabEditScene>();

    prefab_location = std::any_cast<IO::FileLocation>(meta_data);
    const auto prefab_json_str = IO::read_text_file(prefab_location);
    const json prefab_json = json::parse(prefab_json_str);
    Serializer::deserialize(prefab_json, engine.ecs);
}

void PrefabMode::on_switch_away() {}

void PrefabMode::save_prefab() {
    const json prefab_json = Serializer::serialize(engine.ecs);
    const std::string prefab_json_str = prefab_json.dump(4);
    const bool success = IO::write_text_file(prefab_location, prefab_json_str);
    if (!success) {
        // clang-format off
        Notification::create()
            .severity(Severity::ERROR)
            .duration(5.0f)
            .title("Failed to Save Prefab")
            .message("An error occurred while saving the prefab. Check the logs for more details.");
        // clang-format on
    }
}

}  // namespace tmt