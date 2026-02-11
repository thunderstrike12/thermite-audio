#pragma once
#include "engine/core/frame_data.hpp"

namespace tmt {

class IEditorEvents {
   public:
    virtual ~IEditorEvents() = default;
    /* Events */
    /* [Required] */
    virtual void on_editor_start() = 0;
    virtual void on_editor_update(const tmt::FrameData& time) = 0;
    virtual void on_editor_end() = 0;
    /* [Optional] */
    virtual void on_editor_fixed_update(const tmt::FrameData&) {};
};

}  // namespace tmt