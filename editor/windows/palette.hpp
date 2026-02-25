#pragma once

#include "editor/core/window.hpp"

#include "engine/core/resource.hpp"
#include "engine/core/renderer/material.hpp"
#include <imgui.h>

namespace tmt {

class VoxelVolume;

class Palette : public IWindow {
   public:
    constexpr std::string get_title() const override { return ICON_MS_PALETTE " Palette"; }
    constexpr bool default_open() const override { return true; }

    [[nodiscard]] MaterialIndex get_selected_material_index() const { return selected_material_index; }
    void set_selected_material_index(MaterialIndex material_index);

    void before_begin() override;
    void end_display() override;
    void display() override;

    void on_editor_start() override {}
    void on_editor_update(const FrameData&) override {}
    void on_editor_end() override {}

   private:
    friend class MaterialEditor;

    void display_palette(const ResourceRef<VoxelVolume>& resource);

    // Flag used to update the static variable in the material editor when the selected material changes.
    bool update_material_editor = true;
    MaterialIndex selected_material_index { 0 };

    struct Config {
        constexpr static glm::uvec2 GRID_SIZE { 8, 32 };
        constexpr static ImVec2 OUTSET { 2.0f, 2.0f };
    };
};

}  // namespace tmt