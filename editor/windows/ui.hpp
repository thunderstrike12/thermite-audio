#pragma once
#include "editor/core/window.hpp"

namespace tmt {

class UIEditor : public IEditorSystem {
   public:
    UIEditor() = default;
    ~UIEditor() = default;

    void on_editor_update(const FrameData& time) override;
};

}  // namespace tmt