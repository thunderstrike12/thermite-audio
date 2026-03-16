#pragma once
#include <imgui.h>
// Source from: https://github.com/ocornut/imgui/discussions/3862#discussioncomment-8907750
// with some small modifications

namespace tmt {

class CenteredControlWrapperT {
   public:
    explicit CenteredControlWrapperT(bool result) : result(result) {}
    operator bool() const { return result; }

   private:
    bool result;
};

class CenteredControlT {
   public:
    CenteredControlT(ImVec2 window_size, float y = 0.f, float s = 0.f, float su = 0.f) : window_size(window_size), y_offset(y), spacing_below(s), spacing_up(su) {}

    template <typename func>
    CenteredControlWrapperT operator()(func control) const {
        // Measurement pass - render offscreen to get widget size
        ImVec2 original_pos = ImGui::GetCursorPos();
        ImGui::SetCursorPos(ImVec2(-10000.0f, -10000.0f));
        ImGui::PushID("center_measure");
        control();  // discard result
        ImGui::PopID();

        ImVec2 control_size = ImGui::GetItemRectSize();

        // Real render pass - centered, result captured
        ImGui::Dummy(ImVec2(0, spacing_up));
        ImGui::SetCursorPos(ImVec2((window_size.x - control_size.x) * 0.5f, original_pos.y + y_offset));

        bool result = control();

        ImGui::Dummy(ImVec2(0, spacing_below));

        return CenteredControlWrapperT(result);
    }

   private:
    ImVec2 window_size;
    float y_offset;
    float spacing_below;
    float spacing_up;
};

#define IMGUI_CENTER(control, ...) CenteredControlT { ImGui::GetWindowSize(), __VA_ARGS__ }([&]() -> bool { return (control); })

}  // namespace tmt
