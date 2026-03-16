#pragma once

#include "editor/core/window.hpp"

namespace tmt {

class EcsInspector : public IWindow {
   public:
    // Inherited via IWindow
    void on_editor_start() override {};
    void on_editor_update(const tmt::FrameData&) override {};
    void on_editor_end() override {};
    void on_inspect() override;
    constexpr std::string get_title() const override { return "ECS inspector"; };
};

}  // namespace tmt