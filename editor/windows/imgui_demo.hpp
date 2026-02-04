#pragma once
#include "editor/core/window.hpp"
#include <imgui.h>

namespace tmt {
class ImguiDemo : public IWindow {
   public:
    // Inherited via IWindow
    std::string get_title() const override { return "Dear ImGui Demo"; }
    int get_window_flags() const override { return ImGuiWindowFlags_MenuBar; }

    void display() override { ImGui::ShowDemoWindow(); }

    void on_editor_start() override {};
    void on_editor_update(const tmt::FrameData&) override {};
    void on_editor_end() override {};
};
}  // namespace tmt