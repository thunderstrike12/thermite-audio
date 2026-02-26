#pragma once
#include "editor/core/window.hpp"

namespace tmt {

class UIEditor : public IWindow {
   public:
    UIEditor() = default;
    ~UIEditor() = default;

    void display() override;

    void on_editor_update(const FrameData& time) override;

    std::string get_title() const override { return "UI"; };
};

}  // namespace tmt