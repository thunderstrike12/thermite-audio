#pragma once
#include "editor/core/window.hpp"

namespace tmt {
class Inspector : public IWindow {
    // Inherited via IWindow
    std::string get_title() const { return "Inspector"; }

    void display() override;

    void on_editor_start() override;
    void on_editor_update(const tmt::FrameData& time) override;
    void on_editor_end() override;
};
}  // namespace tmt