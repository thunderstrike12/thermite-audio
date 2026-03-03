#pragma once
#include "editor/core/window.hpp"
#include "engine/systems/physics/physics_layers.hpp"

namespace tmt {

/**
 * Class PhysicsLayersEditor
 *
 * Editor window for managing physics layers in the engine.
 *
 * Responsibilities:
 *   - Display existing layers and allow renaming
 *   - Add new layers with names
 *   - Remove layers safely without shifting indices
 *   - Display and edit collision matrix
 *   - Load layers when editor opens and save on close
 *
 * Notes:
 *   - Removing a layer clears its name and disables collisions but preserves its index to maintain consistency with in-game references.
 */
class PhysicsLayersEditor : public IWindow {
   public:
    constexpr std::string get_title() const override { return "Physics Layers"; }

    void on_editor_start() override;
    void on_editor_update(const FrameData&) override {}
    void on_editor_end() override;

    void display() override;

   private:
    void draw_layer_list(PhysicsLayers& layers);
    void draw_collision_matrix(PhysicsLayers& layers);
    void set_layer_name(uint32_t layer, const std::string& name);
};

}  // namespace tmt
