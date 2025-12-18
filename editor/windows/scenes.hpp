#pragma once
#include "editor/core/window.hpp"

namespace tmt {
class ScenesWindow : public IWindow {
    // Inherited via IWindow
    void display() override;

    void on_editor_start() override;
    void on_editor_update(const FrameData& time) override;
    void on_editor_end() override;

    std::string get_title() const override { return "Scenes"; };
};
}  // namespace tmt