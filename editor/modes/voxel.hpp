#pragma once

#include "editor/core/mode.hpp"

#include "engine/core/scene.hpp"
#include "engine/core/components/camera.hpp"
#include "engine/core/components/transform.hpp"
#include "engine/core/renderer/renderer.hpp"

namespace tmt {

class VoxelEditScene : public Scene<VoxelEditScene> {
   public:
    static constexpr std::string_view scene_name() { return "VoxelEditScene"; }
};

class VoxelMode : public IEditorMode {
   public:
    // Inherited via IEditorMode.
    constexpr std::string get_name() override { return "Voxel"; }
    void display_main_menu() override;
    void on_switch_to(const std::any& meta_data = {}) override;
    void on_switch_away() override;

   private:
    Camera cached_editor_camera;
    Transform cached_editor_transform;
    DisplayMode cached_display_mode;

    std::vector<char> edit_data;
    std::map<UUID, Entity> old_entity_mapping;
};

}  // namespace tmt