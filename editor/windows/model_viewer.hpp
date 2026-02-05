#pragma once

#include "editor/core/window.hpp"
#include "editor/windows/brush.hpp"

#include <imgui.h>

#include <engine/core/entity.hpp>
#include <engine/core/resource.hpp>

namespace tmt {

struct Hit;
class VoxelVolume;

class ModelViewer : public IWindow {
   public:
    constexpr std::string get_title() const override { return ICON_MS_DEPLOYED_CODE " Model Viewer"; }
    constexpr int get_window_flags() const override { return ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse; }
    constexpr bool default_open() const override { return true; }

    void display() override;

    void on_editor_start() override {}
    void on_editor_update(const FrameData& time) override;
    void on_editor_end() override {}

   private:
    glm::vec2 mouse_position {};
};

}  // namespace tmt