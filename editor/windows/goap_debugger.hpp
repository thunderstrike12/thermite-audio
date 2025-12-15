#pragma once
#include "editor/core/window.hpp"

namespace tmt {

class GoapDebugger : public IWindow {
   public:
    constexpr std::string get_title() const override { return "GOAP Debugger"; }

    void on_editor_start() override {}
    void on_editor_update(const FrameData&) override {}
    void on_editor_end() override {}

    void display() override;
};

}  // namespace tmt
