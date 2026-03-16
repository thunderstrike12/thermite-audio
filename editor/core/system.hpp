#pragma once
#include "engine/core/frame_data.hpp"

namespace tmt {

class IEditorSystem {
   public:
    virtual ~IEditorSystem() = default;

    /* [Optional] */
    virtual void on_editor_start() {};
    virtual void on_editor_update(const tmt::FrameData& time) { (void)time; };
    virtual void on_editor_fixed_update(const tmt::FrameData& time) { (void)time; };
    virtual void on_editor_end() {};
};

}  // namespace tmt
