#pragma once

#include "editor/core/mode.hpp"

#include "engine/events/scene.hpp"
#include "engine/tools/scene_types.hpp"
#include "engine/core/renderer/renderer.hpp"
#include "engine/core/components/camera.hpp"
#include "engine/core/components/transform.hpp"

namespace tmt {

class SceneMode : public IEditorMode, public OnPreLoadScene {
   public:
    SceneMode();

    // Inherited via IEditorMode.
    constexpr std::string get_name() override { return "Scene"; }
    void display_main_menu() override;
    void on_switch_to(const std::any& meta_data = {}) override;
    void on_switch_away() override;

   private:
    // Inherited via OnPreLoadScene.
    void on_pre_load_scene(PreLoadSceneEvent& event) override;

    SceneIndex previous_scene_type;
    nlohmann::json cached_scene {};

    Camera cached_editor_camera;
    Transform cached_editor_transform;
    DisplayMode cached_display_mode { DisplayMode::DEFAULT };

    // Used as a flag for the on_pre_load_scene function call.
    bool is_switching_to { false };
};

}  // namespace tmt