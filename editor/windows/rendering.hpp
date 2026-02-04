#pragma once

#include "engine/tools/serializer.hpp"
#include "engine/events/debug.hpp"

#include "editor/core/window.hpp"

namespace tmt {

class Rendering : public IWindow, public OnDrawLines {
   public:
    void on_editor_start() override;
    void on_editor_update(const tmt::FrameData& time) override;
    void on_editor_end() override;
    void display() override;
    constexpr std::string get_title() const override { return ICON_MS_MOVIE " Rendering"; }
    void on_draw_lines() const override;
    constexpr std::string get_name() const override { return "Rendering"; };
};

}  // namespace tmt
