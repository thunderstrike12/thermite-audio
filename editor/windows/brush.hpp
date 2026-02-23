#pragma once

#include "editor/core/window.hpp"

namespace tmt {

class Brush : public IWindow {
   public:
    enum class Tool : uint8_t { GIZMO, COLOR_PICKER, SINGLE, BOX };
    enum class Mode : uint8_t { ATTACH, REMOVE, PAINT };

    struct State {
        Tool tool;
        Mode mode;
    };

    Brush() = default;

    [[nodiscard]] State get_brush_state() const { return state; }

   private:
    constexpr std::string get_title() const override { return ICON_MS_BRUSH " Brushes"; }
    constexpr bool default_open() const override { return true; }

    void on_editor_start() override {}
    void on_editor_update(const FrameData&) override {}
    void on_editor_end() override {}

    void display() override;

    State state;
};

}  // namespace tmt