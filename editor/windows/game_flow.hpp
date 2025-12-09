#pragma once
#include "editor/core/window.hpp"

namespace tmt {
class GameFlow : public IWindow {
   public:
    // Inherited via IWindow
    std::string get_title() const override { return "Game Flow"; };

    void display() override;

    void on_editor_start() override;
    void on_editor_update(const tmt::FrameData& time) override;
    void on_editor_end() override;
};
}  // namespace tmt