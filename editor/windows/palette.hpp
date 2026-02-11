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
    void set_selected_material_index(const MaterialIndex material_index) { selected_material_index = material_index; }

    void before_begin() override;
    void end_display() override;
    void display() override;

    void on_editor_start() override {}
    void on_editor_update(const FrameData&) override {}
    void on_editor_end() override {}

   private:
    void display_palette(const ResourceRef<VoxelVolume>& resource);
    void display_material_editor() const;

    MaterialIndex selected_material_index { 0 };

    struct Config {
        constexpr static inline glm::ivec2 GRID_SIZE { 8, 32 };
        constexpr static inline ImVec2 OUTSET { 2.0f, 2.0f };
    };
};

}  // namespace tmt