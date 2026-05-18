#pragma once

#include "editor/core/window.hpp"

#include "engine/core/resources/lut.hpp"

namespace tmt {

class ScreenshotWindow : public IWindow<> {
   public:
    void on_editor_start() override {};
    void on_editor_update(const tmt::FrameData&) override {};
    void on_editor_end() override {};
    void on_inspect() override;
    constexpr std::string get_title() const override { return ICON_MS_VIDEO_CAMERA_FRONT " Screenshot Manager"; }

    ResourceRef<LUT> lut;
};

}  // namespace tmt