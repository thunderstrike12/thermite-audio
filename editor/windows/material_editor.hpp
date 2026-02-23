#pragma once

#include "editor/core/window.hpp"

namespace tmt {

class MaterialEditor : public IWindow {
   public:
    constexpr std::string get_title() const override { return ICON_MS_EDIT " Material Editor"; }
    constexpr bool default_open() const override { return true; }

    void display() override;

    void on_editor_start() override {}
    void on_editor_update(const FrameData&) override {}
    void on_editor_end() override {}

};

}  // namespace tmt