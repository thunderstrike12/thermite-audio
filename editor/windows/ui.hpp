#pragma once
#include "editor/core/window.hpp"
#include "engine/core/reflection.hpp"

namespace tmt {

class UIEditor : public IEditorSystem<> {
   public:
    UIEditor() = default;
    ~UIEditor() = default;

    void on_editor_update(const FrameData& time) override;
};

}  // namespace tmt