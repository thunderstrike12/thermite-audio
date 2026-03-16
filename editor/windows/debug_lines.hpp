#pragma once
#include "editor/core/window.hpp"
#include "engine/events/debug.hpp"

namespace tmt {

class DebugLines : public IWindow {
   public:
    void on_inspect() override;

    virtual void on_editor_update(const tmt::FrameData& time) override;

    constexpr std::string get_title() const override { return "Debug Lines"; };
    constexpr bool default_open() const override { return false; }
};

}  // namespace tmt
